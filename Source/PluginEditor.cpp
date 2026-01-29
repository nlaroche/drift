#include "PluginEditor.h"
#include "ParameterIDs.h"
#include <thread>

DriftEditor::DriftEditor(DriftProcessor& p)
    : AudioProcessorEditor(&p), processor_(p)
{
    setupWebView();
    setupAttachments();

    setSize(900, 600);
    setResizable(false, false);

    startTimerHz(30);
}

DriftEditor::~DriftEditor()
{
    stopTimer();

    timeAttachment_.reset();
    syncAttachment_.reset();
    divisionAttachment_.reset();
    feedbackAttachment_.reset();
    duckAttachment_.reset();
    tapsAttachment_.reset();
    spreadAttachment_.reset();
    mixAttachment_.reset();
    gritAttachment_.reset();
    ageAttachment_.reset();
    diffuseAttachment_.reset();

    webView_.reset();
}

void DriftEditor::setupWebView()
{
    // Create relays BEFORE WebBrowserComponent
    timeRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::time);
    syncRelay_ = std::make_unique<juce::WebToggleButtonRelay>(ParameterIDs::sync);
    divisionRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::division);
    feedbackRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::feedback);
    duckRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::duck);
    tapsRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::taps);
    spreadRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::spread);
    mixRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::mix);
    gritRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::grit);
    ageRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::age);
    diffuseRelay_ = std::make_unique<juce::WebSliderRelay>(ParameterIDs::diffuse);

    auto executableFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    auto executableDir = executableFile.getParentDirectory();

    resourcesDir_ = executableDir.getChildFile("Resources").getChildFile("WebUI");
    if (!resourcesDir_.isDirectory())
        resourcesDir_ = executableDir.getChildFile("WebUI");
    if (!resourcesDir_.isDirectory())
        resourcesDir_ = executableDir.getParentDirectory().getChildFile("Resources").getChildFile("WebUI");

    DBG("Resources dir: " + resourcesDir_.getFullPathName());

    auto options = juce::WebBrowserComponent::Options()
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled()
        .withResourceProvider(
            [this](const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
            {
                auto path = url;
                if (path.startsWith("/")) path = path.substring(1);
                if (path.isEmpty()) path = "index.html";

                auto file = resourcesDir_.getChildFile(path);
                if (!file.existsAsFile()) return std::nullopt;

                juce::String mimeType = "application/octet-stream";
                if (path.endsWith(".html")) mimeType = "text/html";
                else if (path.endsWith(".css")) mimeType = "text/css";
                else if (path.endsWith(".js")) mimeType = "application/javascript";
                else if (path.endsWith(".json")) mimeType = "application/json";
                else if (path.endsWith(".png")) mimeType = "image/png";
                else if (path.endsWith(".svg")) mimeType = "image/svg+xml";
                else if (path.endsWith(".woff2")) mimeType = "font/woff2";

                juce::MemoryBlock data;
                file.loadFileAsData(data);

                return juce::WebBrowserComponent::Resource{
                    std::vector<std::byte>(
                        reinterpret_cast<const std::byte*>(data.getData()),
                        reinterpret_cast<const std::byte*>(data.getData()) + data.getSize()),
                    mimeType.toStdString()
                };
            })
        .withOptionsFrom(*timeRelay_)
        .withOptionsFrom(*syncRelay_)
        .withOptionsFrom(*divisionRelay_)
        .withOptionsFrom(*feedbackRelay_)
        .withOptionsFrom(*duckRelay_)
        .withOptionsFrom(*tapsRelay_)
        .withOptionsFrom(*spreadRelay_)
        .withOptionsFrom(*mixRelay_)
        .withOptionsFrom(*gritRelay_)
        .withOptionsFrom(*ageRelay_)
        .withOptionsFrom(*diffuseRelay_)
        .withEventListener("activateLicense", [this](const juce::var& data) {
            handleActivateLicense(data);
        })
        .withEventListener("deactivateLicense", [this](const juce::var& data) {
            handleDeactivateLicense(data);
        })
        .withEventListener("getActivationStatus", [this](const juce::var&) {
            handleGetActivationStatus();
        })
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2()
                .withBackgroundColour(juce::Colour(0xFF08080c))
                .withStatusBarDisabled()
                .withUserDataFolder(
                    juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("DriftWebView2")));

    webView_ = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*webView_);

#if DRIFT_DEV_MODE
    webView_->goToURL("http://localhost:5173");
#else
    webView_->goToURL(webView_->getResourceProviderRoot());
#endif
}

void DriftEditor::setupAttachments()
{
    auto& apvts = processor_.getAPVTS();

    timeAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::time), *timeRelay_, nullptr);
    syncAttachment_ = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter(ParameterIDs::sync), *syncRelay_, nullptr);
    divisionAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::division), *divisionRelay_, nullptr);
    feedbackAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::feedback), *feedbackRelay_, nullptr);
    duckAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::duck), *duckRelay_, nullptr);
    tapsAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::taps), *tapsRelay_, nullptr);
    spreadAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::spread), *spreadRelay_, nullptr);
    mixAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::mix), *mixRelay_, nullptr);
    gritAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::grit), *gritRelay_, nullptr);
    ageAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::age), *ageRelay_, nullptr);
    diffuseAttachment_ = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter(ParameterIDs::diffuse), *diffuseRelay_, nullptr);
}

void DriftEditor::timerCallback()
{
    juce::DynamicObject::Ptr data = new juce::DynamicObject();
    data->setProperty("inputLevel", processor_.inputLevel.load());
    data->setProperty("duckEnvelope", processor_.duckEnvelope.load());
    data->setProperty("tap1Level", processor_.tap1Level.load());
    data->setProperty("tap2Level", processor_.tap2Level.load());
    data->setProperty("tap3Level", processor_.tap3Level.load());
    data->setProperty("tap4Level", processor_.tap4Level.load());

    webView_->emitEventIfBrowserIsVisible("visualizerData", juce::var(data.get()));
}

void DriftEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF08080c));
}

void DriftEditor::resized()
{
    if (webView_)
        webView_->setBounds(getLocalBounds());
}

// ==============================================================================
// Activation Handlers
// ==============================================================================

void DriftEditor::sendActivationState()
{
    if (webView_ == nullptr) return;

    juce::DynamicObject::Ptr data = new juce::DynamicObject();

#if BEATCONNECT_ACTIVATION_ENABLED
    auto* activation = processor_.getActivation();
    bool isConfigured = (activation != nullptr);
    bool isActivated = isConfigured && activation->isActivated();

    data->setProperty("isConfigured", isConfigured);
    data->setProperty("isActivated", isActivated);

    if (isActivated && activation)
    {
        if (auto info = activation->getActivationInfo())
        {
            juce::DynamicObject::Ptr infoObj = new juce::DynamicObject();
            infoObj->setProperty("activationCode", juce::String(info->activationCode));
            infoObj->setProperty("machineId", juce::String(info->machineId));
            infoObj->setProperty("activatedAt", juce::String(info->activatedAt));
            infoObj->setProperty("currentActivations", info->currentActivations);
            infoObj->setProperty("maxActivations", info->maxActivations);
            infoObj->setProperty("isValid", info->isValid);
            data->setProperty("info", juce::var(infoObj.get()));
        }
    }
#else
    // No activation SDK - skip activation screen
    data->setProperty("isConfigured", false);
    data->setProperty("isActivated", false);
#endif

    webView_->emitEventIfBrowserIsVisible("activationState", juce::var(data.get()));
}

void DriftEditor::handleActivateLicense(const juce::var& data)
{
    juce::String code = data.getProperty("code", "").toString();
    if (code.isEmpty()) return;

#if BEATCONNECT_ACTIVATION_ENABLED
    auto* activation = processor_.getActivation();
    if (!activation) return;

    juce::Component::SafePointer<DriftEditor> safeThis(this);

    activation->activateAsync(code.toStdString(),
        [safeThis](beatconnect::ActivationStatus status) {
            juce::MessageManager::callAsync([safeThis, status]() {
                if (safeThis == nullptr || safeThis->webView_ == nullptr) return;

                juce::DynamicObject::Ptr result = new juce::DynamicObject();

                juce::String statusStr;
                switch (status) {
                    case beatconnect::ActivationStatus::Valid:         statusStr = "valid"; break;
                    case beatconnect::ActivationStatus::Invalid:       statusStr = "invalid"; break;
                    case beatconnect::ActivationStatus::Revoked:       statusStr = "revoked"; break;
                    case beatconnect::ActivationStatus::MaxReached:    statusStr = "max_reached"; break;
                    case beatconnect::ActivationStatus::NetworkError:  statusStr = "network_error"; break;
                    case beatconnect::ActivationStatus::ServerError:   statusStr = "server_error"; break;
                    case beatconnect::ActivationStatus::NotConfigured: statusStr = "not_configured"; break;
                    case beatconnect::ActivationStatus::AlreadyActive: statusStr = "already_active"; break;
                    case beatconnect::ActivationStatus::NotActivated:  statusStr = "not_activated"; break;
                }
                result->setProperty("status", statusStr);

                if (status == beatconnect::ActivationStatus::Valid ||
                    status == beatconnect::ActivationStatus::AlreadyActive)
                {
                    auto* activation = safeThis->processor_.getActivation();
                    if (activation)
                    {
                        if (auto info = activation->getActivationInfo())
                        {
                            juce::DynamicObject::Ptr infoObj = new juce::DynamicObject();
                            infoObj->setProperty("activationCode", juce::String(info->activationCode));
                            infoObj->setProperty("machineId", juce::String(info->machineId));
                            infoObj->setProperty("activatedAt", juce::String(info->activatedAt));
                            infoObj->setProperty("currentActivations", info->currentActivations);
                            infoObj->setProperty("maxActivations", info->maxActivations);
                            infoObj->setProperty("isValid", info->isValid);
                            result->setProperty("info", juce::var(infoObj.get()));
                        }
                    }
                }

                safeThis->webView_->emitEventIfBrowserIsVisible("activationResult", juce::var(result.get()));
            });
        });
#else
    juce::ignoreUnused(data);
#endif
}

void DriftEditor::handleDeactivateLicense(const juce::var&)
{
#if BEATCONNECT_ACTIVATION_ENABLED
    auto* activation = processor_.getActivation();
    if (!activation) return;

    juce::Component::SafePointer<DriftEditor> safeThis(this);

    std::thread([safeThis, activation]() {
        auto status = activation->deactivate();

        juce::MessageManager::callAsync([safeThis, status]() {
            if (safeThis == nullptr || safeThis->webView_ == nullptr) return;

            juce::DynamicObject::Ptr result = new juce::DynamicObject();

            juce::String statusStr;
            switch (status) {
                case beatconnect::ActivationStatus::Valid:         statusStr = "valid"; break;
                case beatconnect::ActivationStatus::NetworkError:  statusStr = "network_error"; break;
                case beatconnect::ActivationStatus::ServerError:   statusStr = "server_error"; break;
                case beatconnect::ActivationStatus::NotActivated:  statusStr = "not_activated"; break;
                default: statusStr = "success"; break;
            }
            result->setProperty("status", statusStr);

            safeThis->webView_->emitEventIfBrowserIsVisible("deactivationResult", juce::var(result.get()));
        });
    }).detach();
#endif
}

void DriftEditor::handleGetActivationStatus()
{
    sendActivationState();
}
