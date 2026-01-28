#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"

DriftProcessor::DriftProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "Parameters", createParameterLayout()),
      rng_(std::random_device{}()),
      dist01_(0.0f, 1.0f)
{
    grainBufferL_.fill(0.0f);
    grainBufferR_.fill(0.0f);
    freezeBufferL_.fill(0.0f);
    freezeBufferR_.fill(0.0f);
    shimmerDelayL_.fill(0.0f);
    shimmerDelayR_.fill(0.0f);
}

DriftProcessor::~DriftProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout DriftProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Grain Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::grainSize, 1), "Grain Size",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.5f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::grainDensity, 1), "Density",
        juce::NormalisableRange<float>(1.0f, 32.0f, 1.0f), 8.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::grainSpread, 1), "Spread",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 25.0f));

    // Pitch Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::pitch, 1), "Pitch",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::pitchScatter, 1), "Pitch Scatter",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::shimmer, 1), "Shimmer",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

    // Time Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::stretch, 1), "Stretch",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.01f, 0.5f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::reverse, 1), "Reverse",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::freeze, 1), "Freeze", false));

    // Texture Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::blur, 1), "Blur",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::warmth, 1), "Warmth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::sparkle, 1), "Sparkle",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f));

    // Output Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::feedback, 1), "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::mix, 1), "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParameterIDs::output, 1), "Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID(ParameterIDs::bypass, 1), "Bypass", false));

    return { params.begin(), params.end() };
}

void DriftProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;

    // Initialize smoothed values
    smoothGrainSize_.reset(sampleRate, 0.05);
    smoothPitch_.reset(sampleRate, 0.05);
    smoothStretch_.reset(sampleRate, 0.1);
    smoothMix_.reset(sampleRate, 0.02);
    smoothOutput_.reset(sampleRate, 0.02);
    smoothFeedback_.reset(sampleRate, 0.05);
    smoothShimmer_.reset(sampleRate, 0.05);

    // Initialize filters
    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 };

    warmthFilterL_.prepare(spec);
    warmthFilterR_.prepare(spec);
    warmthFilterL_.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    warmthFilterR_.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    sparkleFilterL_.prepare(spec);
    sparkleFilterR_.prepare(spec);
    sparkleFilterL_.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    sparkleFilterR_.setType(juce::dsp::StateVariableTPTFilterType::highpass);

    // Clear buffers
    grainBufferL_.fill(0.0f);
    grainBufferR_.fill(0.0f);
    freezeBufferL_.fill(0.0f);
    freezeBufferR_.fill(0.0f);
    shimmerDelayL_.fill(0.0f);
    shimmerDelayR_.fill(0.0f);

    grainBufferWritePos_ = 0;
    shimmerDelayPos_ = 0;
    grainTriggerCounter_ = 0;

    for (auto& grain : grains_)
        grain.active = false;
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

void DriftProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    auto* leftIn = buffer.getReadPointer(0);
    auto* rightIn = buffer.getReadPointer(1);
    auto* leftOut = buffer.getWritePointer(0);
    auto* rightOut = buffer.getWritePointer(1);

    // Get parameters
    const float grainSizeMs = apvts_.getRawParameterValue(ParameterIDs::grainSize)->load();
    const float density = apvts_.getRawParameterValue(ParameterIDs::grainDensity)->load();
    const float spread = apvts_.getRawParameterValue(ParameterIDs::grainSpread)->load() / 100.0f;
    const float pitchSemi = apvts_.getRawParameterValue(ParameterIDs::pitch)->load();
    const float pitchScatter = apvts_.getRawParameterValue(ParameterIDs::pitchScatter)->load() / 100.0f;
    const float shimmerAmt = apvts_.getRawParameterValue(ParameterIDs::shimmer)->load() / 100.0f;
    const float stretch = apvts_.getRawParameterValue(ParameterIDs::stretch)->load();
    const float reverseProb = apvts_.getRawParameterValue(ParameterIDs::reverse)->load() / 100.0f;
    const bool frozen = apvts_.getRawParameterValue(ParameterIDs::freeze)->load() > 0.5f;
    const float blur = apvts_.getRawParameterValue(ParameterIDs::blur)->load() / 100.0f;
    const float warmth = apvts_.getRawParameterValue(ParameterIDs::warmth)->load() / 100.0f;
    const float sparkle = apvts_.getRawParameterValue(ParameterIDs::sparkle)->load() / 100.0f;
    const float feedback = apvts_.getRawParameterValue(ParameterIDs::feedback)->load() / 100.0f;
    const float mixParam = apvts_.getRawParameterValue(ParameterIDs::mix)->load() / 100.0f;
    const float outputDb = apvts_.getRawParameterValue(ParameterIDs::output)->load();
    const bool bypassed = apvts_.getRawParameterValue(ParameterIDs::bypass)->load() > 0.5f;

    if (bypassed)
    {
        grainActivity.store(0.0f);
        return;
    }

    // Update smoothed values
    smoothGrainSize_.setTargetValue(grainSizeMs);
    smoothPitch_.setTargetValue(pitchSemi);
    smoothStretch_.setTargetValue(stretch);
    smoothMix_.setTargetValue(mixParam);
    smoothOutput_.setTargetValue(juce::Decibels::decibelsToGain(outputDb));
    smoothFeedback_.setTargetValue(feedback);
    smoothShimmer_.setTargetValue(shimmerAmt);

    // Update visualizer
    isFrozen.store(frozen);
    currentPitch.store(pitchSemi);

    // Configure filters
    const float warmthFreq = juce::jmap(warmth, 20000.0f, 2000.0f);
    warmthFilterL_.setCutoffFrequency(warmthFreq);
    warmthFilterR_.setCutoffFrequency(warmthFreq);

    const float sparkleFreq = juce::jmap(sparkle, 8000.0f, 2000.0f);
    sparkleFilterL_.setCutoffFrequency(sparkleFreq);
    sparkleFilterR_.setCutoffFrequency(sparkleFreq);

    // Calculate grain trigger interval
    const int grainIntervalSamples = static_cast<int>(sampleRate_ / density);

    // Handle freeze state transition
    if (frozen && !wasFrozen_)
    {
        // Copy current grain buffer to freeze buffer
        freezeBufferL_ = grainBufferL_;
        freezeBufferR_ = grainBufferR_;
    }
    wasFrozen_ = frozen;

    // Shimmer delay time (for octave up effect)
    const int shimmerDelaySamples = static_cast<int>(sampleRate_ * 0.05); // 50ms

    float peakLevel = 0.0f;
    int currentActiveGrains = 0;

    for (int i = 0; i < numSamples; ++i)
    {
        const float currentMix = smoothMix_.getNextValue();
        const float currentOutput = smoothOutput_.getNextValue();
        const float currentFeedback = smoothFeedback_.getNextValue();
        const float currentShimmer = smoothShimmer_.getNextValue();

        // Write input to grain buffer (unless frozen)
        if (!frozen)
        {
            grainBufferL_[grainBufferWritePos_] = leftIn[i];
            grainBufferR_[grainBufferWritePos_] = rightIn[i];
        }

        // Trigger new grains
        grainTriggerCounter_++;
        if (grainTriggerCounter_ >= grainIntervalSamples)
        {
            grainTriggerCounter_ = 0;
            triggerGrain(grainSizeMs, pitchSemi, pitchScatter, reverseProb, spread, frozen);
        }

        // Process all active grains
        float grainOutL = 0.0f;
        float grainOutR = 0.0f;
        currentActiveGrains = 0;

        for (auto& grain : grains_)
        {
            if (!grain.active) continue;
            currentActiveGrains++;

            // Calculate read position with pitch shift
            float readPos;
            if (grain.reverse)
            {
                readPos = static_cast<float>(grain.startSample) -
                         static_cast<float>(grain.currentSample) * grain.pitchRatio;
            }
            else
            {
                readPos = static_cast<float>(grain.startSample) +
                         static_cast<float>(grain.currentSample) * grain.pitchRatio;
            }

            // Wrap read position
            while (readPos < 0) readPos += kGrainBufferSize;
            while (readPos >= kGrainBufferSize) readPos -= kGrainBufferSize;

            // Get grain envelope
            const float phase = static_cast<float>(grain.currentSample) / static_cast<float>(grain.lengthSamples);
            const float envelope = getGrainEnvelope(phase);

            // Read from appropriate buffer
            float sampleL, sampleR;
            if (frozen)
            {
                sampleL = interpolateSample(freezeBufferL_, readPos);
                sampleR = interpolateSample(freezeBufferR_, readPos);
            }
            else
            {
                sampleL = interpolateSample(grainBufferL_, readPos);
                sampleR = interpolateSample(grainBufferR_, readPos);
            }

            // Apply envelope and panning
            const float gainL = envelope * grain.amplitude * (1.0f - grain.pan * 0.5f);
            const float gainR = envelope * grain.amplitude * (0.5f + grain.pan * 0.5f);

            grainOutL += sampleL * gainL;
            grainOutR += sampleR * gainR;

            // Advance grain
            grain.currentSample++;
            if (grain.currentSample >= grain.lengthSamples)
            {
                grain.active = false;
            }
        }

        // Apply blur (simple averaging with feedback)
        if (blur > 0.0f)
        {
            static float blurL = 0.0f, blurR = 0.0f;
            blurL = blurL * blur + grainOutL * (1.0f - blur);
            blurR = blurR * blur + grainOutR * (1.0f - blur);
            grainOutL = blurL;
            grainOutR = blurR;
        }

        // Apply warmth filter
        if (warmth > 0.0f)
        {
            grainOutL = warmthFilterL_.processSample(0, grainOutL);
            grainOutR = warmthFilterR_.processSample(0, grainOutR);
        }

        // Add sparkle (high shelf boost)
        if (sparkle > 0.0f)
        {
            const float sparkleL = sparkleFilterL_.processSample(0, grainOutL);
            const float sparkleR = sparkleFilterR_.processSample(0, grainOutR);
            grainOutL += sparkleL * sparkle * 0.5f;
            grainOutR += sparkleR * sparkle * 0.5f;
        }

        // Shimmer effect (pitch-shifted feedback at octave up)
        if (currentShimmer > 0.0f)
        {
            // Read from shimmer delay with pitch shifting (octave up = 2x speed)
            const int shimmerReadPos = (shimmerDelayPos_ + shimmerDelaySamples) % 96000;
            const float shimmerL = shimmerDelayL_[shimmerReadPos];
            const float shimmerR = shimmerDelayR_[shimmerReadPos];

            grainOutL += shimmerL * currentShimmer * 0.5f;
            grainOutR += shimmerR * currentShimmer * 0.5f;

            // Write to shimmer delay
            shimmerDelayL_[shimmerDelayPos_] = grainOutL * currentFeedback;
            shimmerDelayR_[shimmerDelayPos_] = grainOutR * currentFeedback;
            shimmerDelayPos_ = (shimmerDelayPos_ + 1) % 96000;
        }

        // Feedback into grain buffer
        if (currentFeedback > 0.0f && !frozen)
        {
            grainBufferL_[grainBufferWritePos_] += grainOutL * currentFeedback * 0.3f;
            grainBufferR_[grainBufferWritePos_] += grainOutR * currentFeedback * 0.3f;
        }

        // Advance grain buffer write position
        grainBufferWritePos_ = (grainBufferWritePos_ + 1) % kGrainBufferSize;

        // Mix dry/wet
        const float dryL = leftIn[i];
        const float dryR = rightIn[i];

        leftOut[i] = (dryL * (1.0f - currentMix) + grainOutL * currentMix) * currentOutput;
        rightOut[i] = (dryR * (1.0f - currentMix) + grainOutR * currentMix) * currentOutput;

        peakLevel = std::max(peakLevel, std::abs(leftOut[i]));
        peakLevel = std::max(peakLevel, std::abs(rightOut[i]));
    }

    // Update visualizer data
    grainActivity.store(static_cast<float>(currentActiveGrains) / static_cast<float>(kMaxGrains));
    outputLevel.store(peakLevel);
}

void DriftProcessor::triggerGrain(float grainSizeMs, float pitchSemi, float pitchScatter,
                                   float reverseProb, float spread, bool frozen)
{
    // Find inactive grain slot
    Grain* newGrain = nullptr;
    for (auto& grain : grains_)
    {
        if (!grain.active)
        {
            newGrain = &grain;
            break;
        }
    }

    if (newGrain == nullptr) return;

    // Calculate grain parameters
    const int grainSamples = static_cast<int>(grainSizeMs * sampleRate_ / 1000.0);

    // Random pitch scatter
    float finalPitch = pitchSemi;
    if (pitchScatter > 0.0f)
    {
        finalPitch += (dist01_(rng_) * 2.0f - 1.0f) * pitchScatter * 12.0f;
    }

    // Calculate pitch ratio
    const float pitchRatio = std::pow(2.0f, finalPitch / 12.0f);

    // Random start position (within buffer, offset by spread)
    int startPos;
    if (frozen)
    {
        // When frozen, read from random position in freeze buffer
        startPos = static_cast<int>(dist01_(rng_) * kGrainBufferSize);
    }
    else
    {
        // Normal operation: start near write position with spread
        const int maxOffset = static_cast<int>(spread * kGrainBufferSize * 0.5f);
        const int offset = static_cast<int>((dist01_(rng_) * 2.0f - 1.0f) * maxOffset);
        startPos = (grainBufferWritePos_ - grainSamples + offset + kGrainBufferSize) % kGrainBufferSize;
    }

    // Random reverse
    const bool reverse = dist01_(rng_) < reverseProb;

    // Random pan
    const float pan = dist01_(rng_);

    // Initialize grain
    newGrain->active = true;
    newGrain->startSample = startPos;
    newGrain->currentSample = 0;
    newGrain->lengthSamples = grainSamples;
    newGrain->pitchRatio = pitchRatio;
    newGrain->amplitude = 0.7f + dist01_(rng_) * 0.3f;
    newGrain->reverse = reverse;
    newGrain->pan = pan;
}

float DriftProcessor::getGrainEnvelope(float phase) const
{
    // Hann window for smooth grain envelope
    return 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * phase));
}

float DriftProcessor::interpolateSample(const std::array<float, kGrainBufferSize>& buffer, float pos) const
{
    const int index0 = static_cast<int>(pos) % kGrainBufferSize;
    const int index1 = (index0 + 1) % kGrainBufferSize;
    const float frac = pos - std::floor(pos);

    return buffer[index0] * (1.0f - frac) + buffer[index1] * frac;
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
