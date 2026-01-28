#include "PluginEditor.h"
#include "ParameterIDs.h"

DriftEditor::DriftEditor(DriftProcessor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    setSize(500, 550);
    setResizable(false, false);

    // Create relays BEFORE WebBrowserComponent
    grainSizeRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::grainSize);
    grainDensityRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::grainDensity);
    grainSpreadRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::grainSpread);
    pitchRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::pitch);
    pitchScatterRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::pitchScatter);
    shimmerRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::shimmer);
    stretchRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::stretch);
    reverseRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::reverse);
    freezeRelay_ = std::make_unique<juce::WebToggleButtonRelay>(ParameterIDs::freeze);
    blurRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::blur);
    warmthRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::warmth);
    sparkleRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::sparkle);
    feedbackRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::feedback);
    mixRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::mix);
    outputRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::output);
    bypassRelay_ = std::make_unique<juce::WebToggleButtonRelay>(ParameterIDs::bypass);

    setupWebView();
    startTimerHz(30);
}

DriftEditor::~DriftEditor()
{
    stopTimer();
}

void DriftEditor::setupWebView()
{
    auto options = juce::WebBrowserComponent::Options{}
        .withOptionsFrom(*grainSizeRelay_)
        .withOptionsFrom(*grainDensityRelay_)
        .withOptionsFrom(*grainSpreadRelay_)
        .withOptionsFrom(*pitchRelay_)
        .withOptionsFrom(*pitchScatterRelay_)
        .withOptionsFrom(*shimmerRelay_)
        .withOptionsFrom(*stretchRelay_)
        .withOptionsFrom(*reverseRelay_)
        .withOptionsFrom(*freezeRelay_)
        .withOptionsFrom(*blurRelay_)
        .withOptionsFrom(*warmthRelay_)
        .withOptionsFrom(*sparkleRelay_)
        .withOptionsFrom(*feedbackRelay_)
        .withOptionsFrom(*mixRelay_)
        .withOptionsFrom(*outputRelay_)
        .withOptionsFrom(*bypassRelay_)
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withBackgroundColour(juce::Colour(0xFF0d0d1a))
        );

    webView_ = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*webView_);

    // Create attachments AFTER WebBrowserComponent
    auto& apvts = processor_.getAPVTS();

    grainSizeAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::grainSize), *grainSizeRelay_, nullptr);
    grainDensityAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::grainDensity), *grainDensityRelay_, nullptr);
    grainSpreadAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::grainSpread), *grainSpreadRelay_, nullptr);
    pitchAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::pitch), *pitchRelay_, nullptr);
    pitchScatterAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::pitchScatter), *pitchScatterRelay_, nullptr);
    shimmerAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::shimmer), *shimmerRelay_, nullptr);
    stretchAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::stretch), *stretchRelay_, nullptr);
    reverseAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::reverse), *reverseRelay_, nullptr);
    freezeAttachment_ = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter(ParameterIDs::freeze), *freezeRelay_, nullptr);
    blurAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::blur), *blurRelay_, nullptr);
    warmthAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::warmth), *warmthRelay_, nullptr);
    sparkleAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::sparkle), *sparkleRelay_, nullptr);
    feedbackAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::feedback), *feedbackRelay_, nullptr);
    mixAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::mix), *mixRelay_, nullptr);
    outputAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::output), *outputRelay_, nullptr);
    bypassAttachment_ = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter(ParameterIDs::bypass), *bypassRelay_, nullptr);

    // Load URL
#if DRIFT_DEV_MODE
    webView_->goToURL("http://localhost:5173");
#else
    auto webUIPath = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                         .getParentDirectory().getChildFile("WebUI").getChildFile("index.html");
    webView_->goToURL(juce::URL(webUIPath));
#endif
}

void DriftEditor::timerCallback()
{
    juce::DynamicObject::Ptr data = new juce::DynamicObject();
    data->setProperty("grainActivity", processor_.grainActivity.load());
    data->setProperty("currentPitch", processor_.currentPitch.load());
    data->setProperty("outputLevel", processor_.outputLevel.load());
    data->setProperty("isFrozen", processor_.isFrozen.load());

    webView_->emitEventIfBrowserIsVisible("visualizerData", juce::var(data.get()));
}

void DriftEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF0d0d1a));
}

void DriftEditor::resized()
{
    if (webView_)
        webView_->setBounds(getLocalBounds());
}
