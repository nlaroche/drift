#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <random>

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
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }

    // Visualizer data
    std::atomic<float> grainActivity{ 0.0f };
    std::atomic<float> currentPitch{ 0.0f };
    std::atomic<float> outputLevel{ 0.0f };
    std::atomic<bool> isFrozen{ false };

private:
    juce::AudioProcessorValueTreeState apvts_;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static constexpr int kStateVersion = 1;
    static constexpr int kMaxGrains = 32;
    static constexpr int kGrainBufferSize = 96000; // 2 seconds at 48kHz

    // Grain structure
    struct Grain
    {
        bool active = false;
        int startSample = 0;
        int currentSample = 0;
        int lengthSamples = 0;
        float pitchRatio = 1.0f;
        float amplitude = 1.0f;
        bool reverse = false;
        float pan = 0.5f;
    };

    std::array<Grain, kMaxGrains> grains_;
    std::array<float, kGrainBufferSize> grainBufferL_;
    std::array<float, kGrainBufferSize> grainBufferR_;
    int grainBufferWritePos_ = 0;
    int grainTriggerCounter_ = 0;
    int activeGrainCount_ = 0;

    // Freeze buffer
    std::array<float, kGrainBufferSize> freezeBufferL_;
    std::array<float, kGrainBufferSize> freezeBufferR_;
    bool wasFrozen_ = false;

    // Shimmer delay lines
    std::array<float, 96000> shimmerDelayL_;
    std::array<float, 96000> shimmerDelayR_;
    int shimmerDelayPos_ = 0;

    // Filters
    juce::dsp::StateVariableTPTFilter<float> warmthFilterL_;
    juce::dsp::StateVariableTPTFilter<float> warmthFilterR_;
    juce::dsp::StateVariableTPTFilter<float> sparkleFilterL_;
    juce::dsp::StateVariableTPTFilter<float> sparkleFilterR_;

    // Smoothed parameters
    juce::SmoothedValue<float> smoothGrainSize_;
    juce::SmoothedValue<float> smoothPitch_;
    juce::SmoothedValue<float> smoothStretch_;
    juce::SmoothedValue<float> smoothMix_;
    juce::SmoothedValue<float> smoothOutput_;
    juce::SmoothedValue<float> smoothFeedback_;
    juce::SmoothedValue<float> smoothShimmer_;

    // Random generator
    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist01_;

    double sampleRate_ = 44100.0;

    void triggerGrain(float grainSizeMs, float pitchSemi, float pitchScatter,
                      float reverseProb, float spread, bool frozen);
    float getGrainEnvelope(float phase) const;
    float interpolateSample(const std::array<float, kGrainBufferSize>& buffer, float pos) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriftProcessor)
};
