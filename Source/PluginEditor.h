#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class DriftEditor : public juce::AudioProcessorEditor,
                    private juce::Timer
{
public:
    explicit DriftEditor(DriftProcessor&);
    ~DriftEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupWebView();

    DriftProcessor& processor_;

    // Relays (must be created before WebBrowserComponent)
    std::unique_ptr<juce::WebSliderRelay> grainSizeRelay_;
    std::unique_ptr<juce::WebSliderRelay> grainDensityRelay_;
    std::unique_ptr<juce::WebSliderRelay> grainSpreadRelay_;
    std::unique_ptr<juce::WebSliderRelay> pitchRelay_;
    std::unique_ptr<juce::WebSliderRelay> pitchScatterRelay_;
    std::unique_ptr<juce::WebSliderRelay> shimmerRelay_;
    std::unique_ptr<juce::WebSliderRelay> stretchRelay_;
    std::unique_ptr<juce::WebSliderRelay> reverseRelay_;
    std::unique_ptr<juce::WebToggleButtonRelay> freezeRelay_;
    std::unique_ptr<juce::WebSliderRelay> blurRelay_;
    std::unique_ptr<juce::WebSliderRelay> warmthRelay_;
    std::unique_ptr<juce::WebSliderRelay> sparkleRelay_;
    std::unique_ptr<juce::WebSliderRelay> feedbackRelay_;
    std::unique_ptr<juce::WebSliderRelay> mixRelay_;
    std::unique_ptr<juce::WebSliderRelay> outputRelay_;
    std::unique_ptr<juce::WebToggleButtonRelay> bypassRelay_;

    std::unique_ptr<juce::WebBrowserComponent> webView_;

    // Attachments (created after WebBrowserComponent)
    std::unique_ptr<juce::WebSliderParameterAttachment> grainSizeAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> grainDensityAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> grainSpreadAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchScatterAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> shimmerAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> stretchAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverseAttachment_;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> freezeAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> blurAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> warmthAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> sparkleAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> feedbackAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputAttachment_;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> bypassAttachment_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriftEditor)
};
