#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"
#include <cmath>

#if HAS_PROJECT_DATA
#include "ProjectData.h"
#endif

DriftProcessor::DriftProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "Parameters", createParameterLayout())
{
    loadProjectData();

    delayBufferL_.fill(0.0f);
    delayBufferR_.fill(0.0f);

    for (int i = 0; i < kNumAllpasses; ++i)
    {
        allpassBufferL_[i].fill(0.0f);
        allpassBufferR_[i].fill(0.0f);
        allpassWritePos_[i] = 0;
    }
}

DriftProcessor::~DriftProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout DriftProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // TIME: 10 to 2000 ms
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::time, 1), "Time",
        juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f, 0.4f), 400.0f));

    // SYNC: tempo sync on/off
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::sync, 1), "Sync", false));

    // DIVISION: musical division (0-11)
    // 0=1/1, 1=1/2, 2=1/4, 3=1/8, 4=1/16, 5=1/32, 6=1/4T, 7=1/8T, 8=1/16T, 9=1/4D, 10=1/8D, 11=1/16D
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID(ParameterIDs::division, 1), "Division",
        0, 11, 2)); // Default to 1/4

    // FEEDBACK: 0 to 100%
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::feedback, 1), "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

    // DUCK: 0 to 100%
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::duck, 1), "Duck",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f));

    // TAPS: 1 to 4
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID(ParameterIDs::taps, 1), "Taps",
        1, 4, 2));

    // SPREAD: 0 to 100%
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::spread, 1), "Spread",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

    // MIX: 0 to 100%
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::mix, 1), "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

    // GRIT: 0 to 100% (progressively applied per tap)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::grit, 1), "Grit",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

    // AGE: 0 to 100% (progressively applied per tap)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::age, 1), "Age",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 25.0f));

    // DIFFUSE: 0 to 100% (progressively applied per tap)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::diffuse, 1), "Diffuse",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

    return { params.begin(), params.end() };
}

float DriftProcessor::getTempoSyncedTimeMs(int divisionIndex, double bpm) const
{
    if (bpm <= 0.0) bpm = 120.0;
    if (divisionIndex < 0 || divisionIndex >= static_cast<int>(kDivisionBeats.size()))
        divisionIndex = 2; // Default to 1/4

    // Convert beats to ms: (beats * 60000) / bpm
    const float beats = kDivisionBeats[static_cast<size_t>(divisionIndex)];
    return (beats * 60000.0f) / static_cast<float>(bpm);
}

void DriftProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;

    smoothTime_.reset(sampleRate, 0.05);
    smoothFeedback_.reset(sampleRate, 0.02);
    smoothMix_.reset(sampleRate, 0.02);
    smoothSpread_.reset(sampleRate, 0.02);
    smoothGrit_.reset(sampleRate, 0.02);
    smoothAge_.reset(sampleRate, 0.02);
    smoothDiffuse_.reset(sampleRate, 0.02);

    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };

    hpFilterL_.prepare(spec);
    hpFilterR_.prepare(spec);
    hpFilterL_.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    hpFilterR_.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    hpFilterL_.setCutoffFrequency(60.0f);
    hpFilterR_.setCutoffFrequency(60.0f);

    lpFilterL_.prepare(spec);
    lpFilterR_.prepare(spec);
    lpFilterL_.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    lpFilterR_.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    lpFilterL_.setCutoffFrequency(12000.0f);
    lpFilterR_.setCutoffFrequency(12000.0f);

    delayBufferL_.fill(0.0f);
    delayBufferR_.fill(0.0f);

    writePos_ = 0;
    duckEnv_ = 0.0f;

    ageFilterStateL_.fill(0.0f);
    ageFilterStateR_.fill(0.0f);

    for (int i = 0; i < kNumAllpasses; ++i)
    {
        allpassBufferL_[i].fill(0.0f);
        allpassBufferR_[i].fill(0.0f);
        allpassWritePos_[i] = 0;
        allpassStateL_[i] = 0.0f;
        allpassStateR_[i] = 0.0f;
    }

    driftPhase1_ = 0.0f;
    driftPhase2_ = 0.33f;
    driftPhase3_ = 0.66f;
}

void DriftProcessor::releaseResources() {}

bool DriftProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

float DriftProcessor::readDelayInterp(const std::array<float, kDelayBufferSize>& buffer, float delaySamples) const
{
    float readPos = static_cast<float>(writePos_) - delaySamples;
    while (readPos < 0) readPos += kDelayBufferSize;

    int idx0 = static_cast<int>(readPos) % kDelayBufferSize;
    int idx1 = (idx0 + 1) % kDelayBufferSize;
    float frac = readPos - std::floor(readPos);

    return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
}

float DriftProcessor::processSaturation(float input, float amount) const
{
    if (amount < 0.001f) return input;

    const float drive = 1.0f + amount * 4.0f;
    const float x = input * drive;
    const float saturated = x / (1.0f + std::abs(x));

    return input * (1.0f - amount) + saturated * amount;
}

float DriftProcessor::processAllpass(float input, int index, bool isLeft, float coeff)
{
    auto& buffer = isLeft ? allpassBufferL_[index] : allpassBufferR_[index];
    int& writePos = allpassWritePos_[index];

    const int delayLen = kAllpassDelays[index];
    const int readPos = (writePos - delayLen + 1024) % 1024;

    const float delayed = buffer[readPos];
    const float output = delayed - coeff * input;
    buffer[writePos] = input + coeff * output;

    writePos = (writePos + 1) % 1024;

    return output;
}

void DriftProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    auto* leftIn = buffer.getReadPointer(0);
    auto* rightIn = buffer.getReadPointer(1);
    auto* leftOut = buffer.getWritePointer(0);
    auto* rightOut = buffer.getWritePointer(1);

    // Get parameters
    float timeMs = apvts_.getRawParameterValue(ParameterIDs::time)->load();
    const bool syncEnabled = apvts_.getRawParameterValue(ParameterIDs::sync)->load() > 0.5f;
    const int divisionIdx = static_cast<int>(apvts_.getRawParameterValue(ParameterIDs::division)->load());

    // Get tempo from host if sync is enabled
    if (syncEnabled)
    {
        double bpm = 120.0;
        if (auto* playHead = getPlayHead())
        {
            if (auto posInfo = playHead->getPosition())
            {
                if (posInfo->getBpm().hasValue())
                    bpm = *posInfo->getBpm();
            }
        }
        timeMs = getTempoSyncedTimeMs(divisionIdx, bpm);
    }

    const float feedbackPct = apvts_.getRawParameterValue(ParameterIDs::feedback)->load() / 100.0f;
    const float duckPct = apvts_.getRawParameterValue(ParameterIDs::duck)->load() / 100.0f;
    const int numTaps = static_cast<int>(apvts_.getRawParameterValue(ParameterIDs::taps)->load());
    const float spreadPct = apvts_.getRawParameterValue(ParameterIDs::spread)->load() / 100.0f;
    const float mixPct = apvts_.getRawParameterValue(ParameterIDs::mix)->load() / 100.0f;
    const float gritPct = apvts_.getRawParameterValue(ParameterIDs::grit)->load() / 100.0f;
    const float agePct = apvts_.getRawParameterValue(ParameterIDs::age)->load() / 100.0f;
    const float diffusePct = apvts_.getRawParameterValue(ParameterIDs::diffuse)->load() / 100.0f;

    smoothTime_.setTargetValue(timeMs);
    smoothFeedback_.setTargetValue(feedbackPct);
    smoothMix_.setTargetValue(mixPct);
    smoothSpread_.setTargetValue(spreadPct);
    smoothGrit_.setTargetValue(gritPct);
    smoothAge_.setTargetValue(agePct);
    smoothDiffuse_.setTargetValue(diffusePct);

    const float duckAttack = std::exp(-1.0f / (static_cast<float>(sampleRate_) * 0.005f));
    const float duckRelease = std::exp(-1.0f / (static_cast<float>(sampleRate_) * 0.2f));

    const float driftRate1 = 0.13f / static_cast<float>(sampleRate_);
    const float driftRate2 = 0.089f / static_cast<float>(sampleRate_);
    const float driftRate3 = 0.21f / static_cast<float>(sampleRate_);

    float peakIn = 0.0f;
    std::array<float, 4> tapLevels = { 0.0f, 0.0f, 0.0f, 0.0f };
    float driftViz = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float dryL = leftIn[i];
        const float dryR = rightIn[i];

        const float inputAbs = std::max(std::abs(dryL), std::abs(dryR));
        peakIn = std::max(peakIn, inputAbs);

        if (inputAbs > duckEnv_)
            duckEnv_ = duckAttack * duckEnv_ + (1.0f - duckAttack) * inputAbs;
        else
            duckEnv_ = duckRelease * duckEnv_;

        const float duckGain = 1.0f - std::min(1.0f, duckEnv_ * 2.0f) * duckPct;

        const float currentTime = smoothTime_.getNextValue();
        const float currentFeedback = smoothFeedback_.getNextValue();
        const float currentMix = smoothMix_.getNextValue();
        const float currentSpread = smoothSpread_.getNextValue();
        const float currentGrit = smoothGrit_.getNextValue();
        const float currentAge = smoothAge_.getNextValue();
        const float currentDiffuse = smoothDiffuse_.getNextValue();

        driftPhase1_ += driftRate1;
        driftPhase2_ += driftRate2;
        driftPhase3_ += driftRate3;
        if (driftPhase1_ > 1.0f) driftPhase1_ -= 1.0f;
        if (driftPhase2_ > 1.0f) driftPhase2_ -= 1.0f;
        if (driftPhase3_ > 1.0f) driftPhase3_ -= 1.0f;

        const float drift1 = std::sin(driftPhase1_ * 2.0f * juce::MathConstants<float>::pi);
        const float drift2 = std::sin(driftPhase2_ * 2.0f * juce::MathConstants<float>::pi);
        const float drift3 = std::sin(driftPhase3_ * 2.0f * juce::MathConstants<float>::pi);
        const float driftAmount = (drift1 * 0.5f + drift2 * 0.3f + drift3 * 0.2f);
        driftViz = driftAmount;

        const float driftMod = 1.0f + driftAmount * 0.08f;
        const float baseSamples = currentTime * static_cast<float>(sampleRate_) / 1000.0f;

        float wetL = 0.0f;
        float wetR = 0.0f;

        for (int tap = 0; tap < numTaps; ++tap)
        {
            const float tapDriftMod = 1.0f + driftAmount * 0.08f * (1.0f + tap * 0.3f);
            const float tapMultiplier = static_cast<float>(tap + 1);
            float tapSamples = baseSamples * tapMultiplier * tapDriftMod;
            tapSamples = std::max(1.0f, std::min(tapSamples, static_cast<float>(kDelayBufferSize - 2)));

            const float tapAmp = std::pow(0.7f + currentFeedback * 0.25f, static_cast<float>(tap));

            // Progressive character per tap (later taps get more character)
            const float tapCharacterMult = 0.25f + (static_cast<float>(tap) / 3.0f) * 0.75f;
            const float tapGrit = currentGrit * tapCharacterMult;
            const float tapAge = currentAge * tapCharacterMult;
            const float tapDiffuse = currentDiffuse * tapCharacterMult;

            const float spreadAmount = currentSpread * (static_cast<float>(tap) / 3.0f);
            float panL, panR;
            if (tap % 2 == 0) {
                panL = 1.0f;
                panR = 1.0f - spreadAmount * 0.7f;
            } else {
                panL = 1.0f - spreadAmount * 0.7f;
                panR = 1.0f;
            }

            float tapL = readDelayInterp(delayBufferL_, tapSamples);
            float tapR = readDelayInterp(delayBufferR_, tapSamples);

            // Per-tap age filtering
            if (tapAge > 0.001f)
            {
                const float ageCoeff = 1.0f - tapAge * 0.7f;
                ageFilterStateL_[tap] = ageFilterStateL_[tap] + ageCoeff * (tapL - ageFilterStateL_[tap]);
                ageFilterStateR_[tap] = ageFilterStateR_[tap] + ageCoeff * (tapR - ageFilterStateR_[tap]);
                tapL = ageFilterStateL_[tap];
                tapR = ageFilterStateR_[tap];
            }

            // Per-tap saturation
            tapL = processSaturation(tapL, tapGrit);
            tapR = processSaturation(tapR, tapGrit);

            // Per-tap diffusion (using different allpass indices per tap)
            if (tapDiffuse > 0.001f && tap < kNumAllpasses)
            {
                const float diffCoeff = 0.5f + tapDiffuse * 0.35f;
                tapL = processAllpass(tapL, tap, true, diffCoeff);
                tapR = processAllpass(tapR, tap, false, diffCoeff);
            }

            wetL += tapL * tapAmp * panL;
            wetR += tapR * tapAmp * panR;

            tapLevels[tap] = std::max(tapLevels[tap], (std::abs(tapL) + std::abs(tapR)) * 0.5f * tapAmp);
        }

        // Global filters
        wetL = hpFilterL_.processSample(0, wetL);
        wetR = hpFilterR_.processSample(0, wetR);
        wetL = lpFilterL_.processSample(0, wetL);
        wetR = lpFilterR_.processSample(0, wetR);

        wetL *= duckGain;
        wetR *= duckGain;

        // Feedback path (with global grit for self-oscillation character)
        float fbL = readDelayInterp(delayBufferL_, baseSamples * driftMod);
        float fbR = readDelayInterp(delayBufferR_, baseSamples * driftMod);

        fbL = processSaturation(fbL, currentGrit * 0.5f);
        fbR = processSaturation(fbR, currentGrit * 0.5f);

        delayBufferL_[writePos_] = dryL + fbL * currentFeedback;
        delayBufferR_[writePos_] = dryR + fbR * currentFeedback;

        writePos_ = (writePos_ + 1) % kDelayBufferSize;

        leftOut[i] = dryL * (1.0f - currentMix) + wetL * currentMix;
        rightOut[i] = dryR * (1.0f - currentMix) + wetR * currentMix;
    }

    inputLevel.store(peakIn);
    duckEnvelope.store(driftViz);
    tap1Level.store(tapLevels[0]);
    tap2Level.store(tapLevels[1]);
    tap3Level.store(tapLevels[2]);
    tap4Level.store(tapLevels[3]);
}

juce::AudioProcessorEditor* DriftProcessor::createEditor()
{
    return new DriftEditor(*this);
}

void DriftProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts_.copyState();
    state.setProperty("stateVersion", kStateVersion, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DriftProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts_.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        apvts_.replaceState(state);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DriftProcessor();
}

// ==============================================================================
// BeatConnect Integration
// ==============================================================================

void DriftProcessor::loadProjectData()
{
#if HAS_PROJECT_DATA
    int dataSize = 0;
    const char* data = ProjectData::getNamedResource("project_data_json", dataSize);

    if (data == nullptr || dataSize == 0)
    {
        DBG("No project_data.json found in binary data");
        return;
    }

    auto parsed = juce::JSON::parse(juce::String::fromUTF8(data, dataSize));
    if (parsed.isVoid())
    {
        DBG("Failed to parse project_data.json");
        return;
    }

    pluginId_ = parsed.getProperty("pluginId", "").toString();
    apiBaseUrl_ = parsed.getProperty("apiBaseUrl", "").toString();
    supabaseKey_ = parsed.getProperty("supabasePublishableKey", "").toString();
    buildFlags_ = parsed.getProperty("flags", juce::var());

    DBG("Loaded project data - pluginId: " + pluginId_);

#if BEATCONNECT_ACTIVATION_ENABLED
    bool enableActivation = static_cast<bool>(buildFlags_.getProperty("enableActivationKeys", false));

    if (enableActivation && pluginId_.isNotEmpty())
    {
        beatconnect::ActivationConfig config;
        config.apiBaseUrl = apiBaseUrl_.toStdString();
        config.pluginId = pluginId_.toStdString();
        config.supabaseKey = supabaseKey_.toStdString();
        config.validateOnStartup = true;
        config.revalidateIntervalSeconds = 86400; // Daily revalidation

        activation_ = beatconnect::Activation::create(config);
        DBG("Activation system configured");
    }
#endif
#endif
}

bool DriftProcessor::hasActivationEnabled() const
{
#if HAS_PROJECT_DATA && BEATCONNECT_ACTIVATION_ENABLED
    return static_cast<bool>(buildFlags_.getProperty("enableActivationKeys", false));
#else
    return false;
#endif
}
