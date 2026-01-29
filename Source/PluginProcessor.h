#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

#if BEATCONNECT_ACTIVATION_ENABLED
#include <beatconnect/Activation.h>
#endif

class DriftProcessor : public juce::AudioProcessor
{
public:
    DriftProcessor();
    ~DriftProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }

    // BeatConnect integration
    bool hasActivationEnabled() const;

#if BEATCONNECT_ACTIVATION_ENABLED
    beatconnect::Activation* getActivation() { return activation_.get(); }
#endif

    // Visualizer data
    std::atomic<float> inputLevel{ 0.0f };
    std::atomic<float> duckEnvelope{ 0.0f };
    std::atomic<float> tap1Level{ 0.0f };
    std::atomic<float> tap2Level{ 0.0f };
    std::atomic<float> tap3Level{ 0.0f };
    std::atomic<float> tap4Level{ 0.0f };

private:
    juce::AudioProcessorValueTreeState apvts_;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void loadProjectData();

    static constexpr int kStateVersion = 6;

    // BeatConnect data
    juce::String pluginId_;
    juce::String apiBaseUrl_;
    juce::String supabaseKey_;
    juce::var buildFlags_;

#if BEATCONNECT_ACTIVATION_ENABLED
    std::unique_ptr<beatconnect::Activation> activation_;
#endif

    // Musical divisions in beats (relative to quarter note)
    static constexpr std::array<float, 12> kDivisionBeats = {
        4.0f,    // 1/1
        2.0f,    // 1/2
        1.0f,    // 1/4
        0.5f,    // 1/8
        0.25f,   // 1/16
        0.125f,  // 1/32
        1.333333f, // 1/4T (triplet)
        0.666667f, // 1/8T
        0.333333f, // 1/16T
        1.5f,    // 1/4D (dotted)
        0.75f,   // 1/8D
        0.375f   // 1/16D
    };

    float getTempoSyncedTimeMs(int divisionIndex, double bpm) const;
    static constexpr int kDelayBufferSize = 192000;

    // Stereo delay buffers
    std::array<float, kDelayBufferSize> delayBufferL_{};
    std::array<float, kDelayBufferSize> delayBufferR_{};
    int writePos_ = 0;

    // Ducking envelope follower
    float duckEnv_ = 0.0f;

    // Highpass filter for delay (removes mud)
    juce::dsp::StateVariableTPTFilter<float> hpFilterL_;
    juce::dsp::StateVariableTPTFilter<float> hpFilterR_;

    // Lowpass for smoothing
    juce::dsp::StateVariableTPTFilter<float> lpFilterL_;
    juce::dsp::StateVariableTPTFilter<float> lpFilterR_;

    // Smoothed parameters
    juce::SmoothedValue<float> smoothTime_;
    juce::SmoothedValue<float> smoothFeedback_;
    juce::SmoothedValue<float> smoothMix_;
    juce::SmoothedValue<float> smoothSpread_;
    juce::SmoothedValue<float> smoothGrit_;
    juce::SmoothedValue<float> smoothAge_;
    juce::SmoothedValue<float> smoothDiffuse_;

    double sampleRate_ = 44100.0;

    // Drift LFO phases
    float driftPhase1_ = 0.0f;
    float driftPhase2_ = 0.0f;
    float driftPhase3_ = 0.0f;

    // Age filter state per tap (for progressive darkening)
    std::array<float, 4> ageFilterStateL_{};
    std::array<float, 4> ageFilterStateR_{};

    // Diffusion allpass states (4 allpasses in series)
    static constexpr int kNumAllpasses = 4;
    std::array<float, kNumAllpasses> allpassStateL_{};
    std::array<float, kNumAllpasses> allpassStateR_{};
    static constexpr std::array<int, kNumAllpasses> kAllpassDelays = { 113, 199, 421, 677 }; // Prime numbers for diffusion
    std::array<std::array<float, 1024>, kNumAllpasses> allpassBufferL_{};
    std::array<std::array<float, 1024>, kNumAllpasses> allpassBufferR_{};
    std::array<int, kNumAllpasses> allpassWritePos_{};

    float readDelayInterp(const std::array<float, kDelayBufferSize>& buffer, float delaySamples) const;
    float processSaturation(float input, float amount) const;
    float processAllpass(float input, int index, bool isLeft, float coeff);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriftProcessor)
};
