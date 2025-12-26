#ifndef MENU_SETTINGS_CONTROLLER_H
#define MENU_SETTINGS_CONTROLLER_H

#include "../MenuConstants.h"
#include "SettingsManager.h"
#include "scene/camera/ICameraSensitivityController.h"

#include <functional>
#include <imgui.h>
#include <string>
#include <vector>

// Forward declarations

struct VideoSettings
{
    int resolutionIndex = 1;
    int displayModeIndex = 0;
    int vsyncIndex = 1;
    int fpsIndex = 1;
};

// Callback for navigation
using BackCallback = std::function<void()>;

class MenuSettingsController
{
public:
    MenuSettingsController();
    ~MenuSettingsController() = default;

    // Initialization
    void Initialize(SettingsManager *settingsManager,
                    ICameraSensitivityController *cameraController);

    // Set callback for Back button
    void SetBackCallback(BackCallback callback);

    // Rendering methods
    void RenderVideoSettings();
    void RenderAudioSettings();
    void RenderControlSettings();

    // Settings management
    void ApplyPendingSettings();
    bool HasUnsavedVideoChanges() const;

    // Camera controller dependency injection
    void SetCameraController(ICameraSensitivityController *controller);

private:
    // Settings synchronization
    void SyncVideoSettingsToConfig() const;
    void SyncAudioSettingsToConfig() const;
    void SyncControlSettingsToConfig();
    void ApplyCameraSensitivity(float sensitivity);

    // UI helpers
    bool RenderVideoSettingCombo(const char *label, const char *id,
                                 const std::vector<std::string> &options, int &currentIndex,
                                 float labelWidth, float comboWidth, float startX);

    // Dependencies
    SettingsManager *m_settingsManager = nullptr;
    ICameraSensitivityController *m_cameraController = nullptr;

    // Settings state
    VideoSettings m_videoSettings{};
    AudioSettings m_audioSettings{};
    ControlSettings m_controlSettings{};

    // Options vectors
    std::vector<std::string> m_resolutionOptions;
    std::vector<std::string> m_displayModeOptions;
    std::vector<std::string> m_vsyncOptions;
    std::vector<std::string> m_fpsOptions;

    // Navigation callback
    BackCallback m_backCallback;
};

#endif // MENU_SETTINGS_CONTROLLER_H
