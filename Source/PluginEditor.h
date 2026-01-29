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
    void setupAttachments();

    DriftProcessor& processor_;

    juce::File resourcesDir_;

    // Relays - 11 params for wandering delay
    std::unique_ptr<juce::WebSliderRelay> timeRelay_;
    std::unique_ptr<juce::WebToggleButtonRelay> syncRelay_;
    std::unique_ptr<juce::WebSliderRelay> divisionRelay_;
    std::unique_ptr<juce::WebSliderRelay> feedbackRelay_;
    std::unique_ptr<juce::WebSliderRelay> duckRelay_;
    std::unique_ptr<juce::WebSliderRelay> tapsRelay_;
    std::unique_ptr<juce::WebSliderRelay> spreadRelay_;
    std::unique_ptr<juce::WebSliderRelay> mixRelay_;
    std::unique_ptr<juce::WebSliderRelay> gritRelay_;
    std::unique_ptr<juce::WebSliderRelay> ageRelay_;
    std::unique_ptr<juce::WebSliderRelay> diffuseRelay_;

    std::unique_ptr<juce::WebBrowserComponent> webView_;

    // Attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> timeAttachment_;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> syncAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> divisionAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> feedbackAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> duckAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> tapsAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> spreadAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> gritAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> ageAttachment_;
    std::unique_ptr<juce::WebSliderParameterAttachment> diffuseAttachment_;

    // Activation handlers
    void sendActivationState();
    void handleActivateLicense(const juce::var& data);
    void handleDeactivateLicense(const juce::var& data);
    void handleGetActivationStatus();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriftEditor)
};
