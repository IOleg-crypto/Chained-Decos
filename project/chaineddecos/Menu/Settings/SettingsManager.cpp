#include "SettingsManager.h"
#include "MenuConstants.h"
#include <raylib.h>
#include <iostream>
#include <algorithm>

SettingsManager::SettingsManager() {
    LoadSettings();
}

void SettingsManager::LoadSettings() {
    // Load configuration from the single canonical file: game.cfg (current directory)
    bool loaded = false;
    if (m_config.LoadFromFile("game.cfg")) {
        TraceLog(LOG_INFO, "SettingsManager::LoadSettings() - Loaded game.cfg from current directory");
        loaded = true;
    } else {
        TraceLog(LOG_WARNING, "SettingsManager::LoadSettings() - Could not load game.cfg, using default settings");
    }


    // NOTE: We do NOT apply window settings here (SetWindowSize, SetWindowState)
    // because this is called from the constructor, before the window exists.
    // Window settings will be applied by GameApplication::OnConfigure() and OnStart()

    // Load audio settings
    m_audioSettings.masterVolume = m_config.GetMasterVolume();
    m_audioSettings.musicVolume = m_config.GetMusicVolume();
    m_audioSettings.sfxVolume = m_config.GetSFXVolume();
    // Note: ConfigManager doesn't have IsAudioMuted, using default for now
    m_audioSettings.muted = false;

    // Load control settings
    m_controlSettings.mouseSensitivity = m_config.GetMouseSensitivity();
    m_controlSettings.invertYAxis = m_config.GetInvertY();
    // Note: ConfigManager doesn't have IsControllerSupportEnabled, using default for now
    m_controlSettings.controllerSupport = true;

    // Load parkour-specific settings
    m_parkourSettings.wallRunSensitivity = m_config.GetWallRunSensitivity();
    m_parkourSettings.jumpTiming = m_config.GetJumpTiming();
    m_parkourSettings.slideControl = m_config.GetSlideControl();
    m_parkourSettings.grappleSensitivity = m_config.GetGrappleSensitivity();

    // Load gameplay settings
    m_gameplaySettings.difficultyLevel = m_config.GetDifficultyLevel();
    m_gameplaySettings.timerEnabled = m_config.IsTimerEnabled();
    m_gameplaySettings.checkpointsEnabled = m_config.AreCheckpointsEnabled();
    m_gameplaySettings.autoSaveEnabled = m_config.IsAutoSaveEnabled();
    m_gameplaySettings.speedrunMode = m_config.IsSpeedrunMode();

    m_gameplaySettings.wallRunEnabled = m_config.IsWallRunEnabled();
    m_gameplaySettings.doubleJumpEnabled = m_config.IsDoubleJumpEnabled();
    m_gameplaySettings.slideEnabled = m_config.IsSlideEnabled();
    m_gameplaySettings.grappleEnabled = m_config.IsGrappleEnabled();
    m_gameplaySettings.slowMotionOnTrick = m_config.IsSlowMotionOnTrick();

    // Load video settings indices based on current configuration
    // This is a simplified approach - in a real implementation you'd want to match
    // the current resolution to the available options
    m_currentResolutionIndex = 1; // Default to 1280x720
    m_currentAspectRatioIndex = 0; // Default to 16:9
    m_currentDisplayModeIndex = m_config.IsFullscreen() ? 1 : 0;
    m_currentVSyncIndex = m_config.IsVSync() ? 1 : 0;
    m_currentFpsIndex = 1; // Default to 60 FPS
}

void SettingsManager::SaveSettings() {
    // Save current window settings
    m_config.SetResolution(GetScreenWidth(), GetScreenHeight());
    m_config.SetFullscreen(IsWindowFullscreen());
    m_config.SetVSync(IsWindowState(FLAG_VSYNC_HINT));

    // Save audio settings
    m_config.SetMasterVolume(m_audioSettings.masterVolume);
    m_config.SetMusicVolume(m_audioSettings.musicVolume);
    m_config.SetSFXVolume(m_audioSettings.sfxVolume);

    // Save control settings
    m_config.SetMouseSensitivity(m_controlSettings.mouseSensitivity);
    m_config.SetInvertY(m_controlSettings.invertYAxis);

    // Save parkour-specific settings
    m_config.SetWallRunSensitivity(m_parkourSettings.wallRunSensitivity);
    m_config.SetJumpTiming(m_parkourSettings.jumpTiming);
    m_config.SetSlideControl(m_parkourSettings.slideControl);
    m_config.SetGrappleSensitivity(m_parkourSettings.grappleSensitivity);

    // Save gameplay settings
    m_config.SetDifficultyLevel(m_gameplaySettings.difficultyLevel);
    m_config.SetTimerEnabled(m_gameplaySettings.timerEnabled);
    m_config.SetCheckpointsEnabled(m_gameplaySettings.checkpointsEnabled);
    m_config.SetAutoSaveEnabled(m_gameplaySettings.autoSaveEnabled);
    m_config.SetSpeedrunMode(m_gameplaySettings.speedrunMode);

    m_config.SetWallRunEnabled(m_gameplaySettings.wallRunEnabled);
    m_config.SetDoubleJumpEnabled(m_gameplaySettings.doubleJumpEnabled);
    m_config.SetSlideEnabled(m_gameplaySettings.slideEnabled);
    m_config.SetGrappleEnabled(m_gameplaySettings.grappleEnabled);
    m_config.SetSlowMotionOnTrick(m_gameplaySettings.slowMotionOnTrick);

    // Save to file (single canonical location: game.cfg)
    if (m_config.SaveToFile("game.cfg")) {
        TraceLog(LOG_INFO, "SettingsManager::SaveSettings() - Settings saved to game.cfg");
    } else {
        TraceLog(LOG_WARNING, "SettingsManager::SaveSettings() - Failed to save settings to game.cfg");
    }
}

void SettingsManager::ApplyVideoSettings() {
    TraceLog(LOG_INFO, "SettingsManager::ApplyVideoSettings() - Applying video settings");

    // Get resolution from options
    auto& resolutionOptions = MenuConstants::RESOLUTION_OPTIONS;
    if (m_currentResolutionIndex >= 0 && m_currentResolutionIndex < static_cast<int>(resolutionOptions.size())) {
        auto resolution = resolutionOptions[m_currentResolutionIndex];
        TraceLog(LOG_INFO, "SettingsManager::ApplyVideoSettings() - Setting resolution to: %s", resolution.c_str());

        // Parse resolution string (e.g., "1920x1080")
        size_t xPos = resolution.find('x');
        if (xPos != std::string::npos) {
            int width = std::stoi(resolution.substr(0, xPos));
            int height = std::stoi(resolution.substr(xPos + 1));

            // Only apply if window is not already the target size (to avoid unnecessary operations)
            if (GetScreenWidth() != width || GetScreenHeight() != height) {
                TraceLog(LOG_INFO, "SettingsManager::ApplyVideoSettings() - Changing window size from %dx%d to %dx%d",
                    GetScreenWidth(), GetScreenHeight(), width, height);
                SetWindowSize(width, height);
            }
        }
    }

    // Apply display mode (Windowed/Fullscreen/Borderless)
    bool isCurrentlyFullscreen = IsWindowFullscreen();
    bool isCurrentlyBorderless = IsWindowState(FLAG_WINDOW_UNDECORATED);
    
    // 0 = Windowed, 1 = Fullscreen, 2 = Borderless
    bool shouldBeFullscreen = (m_currentDisplayModeIndex == 1);
    bool shouldBeBorderless = (m_currentDisplayModeIndex == 2);

    // Handle fullscreen
    if (shouldBeFullscreen && !isCurrentlyFullscreen) {
        TraceLog(LOG_INFO, "SettingsManager::ApplyVideoSettings() - Enabling fullscreen mode");
        // Clear borderless first if active
        if (isCurrentlyBorderless) {
            ClearWindowState(FLAG_WINDOW_UNDECORATED);
        }
        SetWindowState(FLAG_FULLSCREEN_MODE);
    } else if (!shouldBeFullscreen && isCurrentlyFullscreen) {
        TraceLog(LOG_INFO, "SettingsManager::ApplyVideoSettings() - Disabling fullscreen mode");
        ClearWindowState(FLAG_FULLSCREEN_MODE);
        // Apply borderless if needed
        if (shouldBeBorderless) {
            SetWindowState(FLAG_WINDOW_UNDECORATED);
        }
    }
    
    // Handle borderless (only when not fullscreen)
    if (!shouldBeFullscreen) {
        if (shouldBeBorderless && !isCurrentlyBorderless) {
            TraceLog(LOG_INFO, "SettingsManager::ApplyVideoSettings() - Enabling borderless mode");
            SetWindowState(FLAG_WINDOW_UNDECORATED);
        } else if (!shouldBeBorderless && isCurrentlyBorderless) {
            TraceLog(LOG_INFO, "SettingsManager::ApplyVideoSettings() - Disabling borderless mode");
            ClearWindowState(FLAG_WINDOW_UNDECORATED);
        }
    }

    // Apply VSync
    bool isCurrentlyVSync = IsWindowState(FLAG_VSYNC_HINT);
    bool shouldBeVSync = (m_currentVSyncIndex == 1);

    if (shouldBeVSync && !isCurrentlyVSync) {
        TraceLog(LOG_INFO, "SettingsManager::ApplyVideoSettings() - Enabling VSync");
        SetWindowState(FLAG_VSYNC_HINT);
    } else if (!shouldBeVSync && isCurrentlyVSync) {
        TraceLog(LOG_INFO, "SettingsManager::ApplyVideoSettings() - Disabling VSync");
        ClearWindowState(FLAG_VSYNC_HINT);
    }

    // Apply FPS target
    auto& fpsOptions = MenuConstants::FPS_OPTIONS;
    if (m_currentFpsIndex >= 0 && m_currentFpsIndex < static_cast<int>(fpsOptions.size())) {
        auto fps = fpsOptions[m_currentFpsIndex];
        int targetFps = (fps == "Unlimited") ? 0 : std::stoi(fps);

        // Set target FPS (raylib doesn't have GetTargetFPS, so we always set it)
        TraceLog(LOG_INFO, "SettingsManager::ApplyVideoSettings() - Setting target FPS to: %s", fps.c_str());
        SetTargetFPS(targetFps);
    }

    TraceLog(LOG_INFO, "SettingsManager::ApplyVideoSettings() - Video settings applied successfully");
}

void SettingsManager::ApplyAudioSettings() {
    TraceLog(LOG_INFO, "SettingsManager::ApplyAudioSettings() - Applying audio settings");

    if (m_audioManager) {
        // Apply master volume
        float effectiveMaster = m_audioSettings.muted ? 0.0f : m_audioSettings.masterVolume;
        m_audioManager->SetMasterVolume(effectiveMaster);
        TraceLog(LOG_INFO, "SettingsManager::ApplyAudioSettings() - Master volume: %.2f (muted: %s)", effectiveMaster, m_audioSettings.muted ? "true" : "false");

        // Apply music volume
        m_audioManager->SetMusicVolume(m_audioSettings.musicVolume);
        TraceLog(LOG_INFO, "SettingsManager::ApplyAudioSettings() - Music volume: %.2f", m_audioSettings.musicVolume);

        // Apply SFX volume
        m_audioManager->SetSoundVolume(m_audioSettings.sfxVolume);
        TraceLog(LOG_INFO, "SettingsManager::ApplyAudioSettings() - SFX volume: %.2f", m_audioSettings.sfxVolume);

        TraceLog(LOG_INFO, "SettingsManager::ApplyAudioSettings() - Audio settings applied to AudioManager successfully");
    } else {
        TraceLog(LOG_WARNING, "SettingsManager::ApplyAudioSettings() - AudioManager not set, cannot apply audio settings");
    }
}

// Audio settings methods
void SettingsManager::SetMasterVolume(float volume) {
    m_audioSettings.masterVolume = std::max(0.0f, std::min(1.0f, volume));
}

void SettingsManager::SetMusicVolume(float volume) {
    m_audioSettings.musicVolume = std::max(0.0f, std::min(1.0f, volume));
}

void SettingsManager::SetSfxVolume(float volume) {
    m_audioSettings.sfxVolume = std::max(0.0f, std::min(1.0f, volume));
}

void SettingsManager::SetMuted(bool muted) {
    m_audioSettings.muted = muted;
}

// Control settings methods
void SettingsManager::SetMouseSensitivity(float sensitivity) {
    m_controlSettings.mouseSensitivity = std::max(0.1f, std::min(5.0f, sensitivity));
}

void SettingsManager::SetInvertYAxis(bool invert) {
    m_controlSettings.invertYAxis = invert;
}

void SettingsManager::SetControllerSupport(bool enabled) {
    m_controlSettings.controllerSupport = enabled;
}

// Parkour control settings methods
void SettingsManager::SetWallRunSensitivity(float sensitivity) {
    m_parkourSettings.wallRunSensitivity = std::max(0.1f, std::min(5.0f, sensitivity));
}

void SettingsManager::SetJumpTiming(float timing) {
    m_parkourSettings.jumpTiming = std::max(0.1f, std::min(5.0f, timing));
}

void SettingsManager::SetSlideControl(float control) {
    m_parkourSettings.slideControl = std::max(0.1f, std::min(5.0f, control));
}

void SettingsManager::SetGrappleSensitivity(float sensitivity) {
    m_parkourSettings.grappleSensitivity = std::max(0.1f, std::min(5.0f, sensitivity));
}

// Gameplay settings methods
void SettingsManager::SetDifficultyLevel(int level) {
    m_gameplaySettings.difficultyLevel = std::max(0, std::min(2, level));
}

void SettingsManager::SetTimerEnabled(bool enabled) {
    m_gameplaySettings.timerEnabled = enabled;
}

void SettingsManager::SetCheckpointsEnabled(bool enabled) {
    m_gameplaySettings.checkpointsEnabled = enabled;
}

void SettingsManager::SetAutoSaveEnabled(bool enabled) {
    m_gameplaySettings.autoSaveEnabled = enabled;
}

void SettingsManager::SetSpeedrunMode(bool enabled) {
    m_gameplaySettings.speedrunMode = enabled;
}

void SettingsManager::SetWallRunEnabled(bool enabled) {
    m_gameplaySettings.wallRunEnabled = enabled;
}

void SettingsManager::SetDoubleJumpEnabled(bool enabled) {
    m_gameplaySettings.doubleJumpEnabled = enabled;
}

void SettingsManager::SetSlideEnabled(bool enabled) {
    m_gameplaySettings.slideEnabled = enabled;
}

void SettingsManager::SetGrappleEnabled(bool enabled) {
    m_gameplaySettings.grappleEnabled = enabled;
}

void SettingsManager::SetSlowMotionOnTrick(bool enabled) {
    m_gameplaySettings.slowMotionOnTrick = enabled;
}

// Video settings methods
void SettingsManager::SetResolutionIndex(int index) {
    m_currentResolutionIndex = std::max(0, std::min(static_cast<int>(MenuConstants::RESOLUTION_OPTIONS.size()) - 1, index));
}

void SettingsManager::SetAspectRatioIndex(int index) {
    m_currentAspectRatioIndex = std::max(0, std::min(static_cast<int>(MenuConstants::ASPECT_RATIO_OPTIONS.size()) - 1, index));
}

void SettingsManager::SetDisplayModeIndex(int index) {
    m_currentDisplayModeIndex = std::max(0, std::min(static_cast<int>(MenuConstants::DISPLAY_MODE_OPTIONS.size()) - 1, index));
}

void SettingsManager::SetVSyncIndex(int index) {
    m_currentVSyncIndex = std::max(0, std::min(static_cast<int>(MenuConstants::VSYNC_OPTIONS.size()) - 1, index));
}

void SettingsManager::SetFpsIndex(int index) {
    m_currentFpsIndex = std::max(0, std::min(static_cast<int>(MenuConstants::FPS_OPTIONS.size()) - 1, index));
}

std::string SettingsManager::GetCurrentSettingValue(const std::string& settingName) const {
    if (settingName == "Master Volume") {
        return std::to_string(static_cast<int>(m_audioSettings.masterVolume * 100));
    } else if (settingName == "Music Volume") {
        return std::to_string(static_cast<int>(m_audioSettings.musicVolume * 100));
    } else if (settingName == "SFX Volume") {
        return std::to_string(static_cast<int>(m_audioSettings.sfxVolume * 100));
    } else if (settingName == "Mouse Sensitivity") {
        return std::to_string(static_cast<int>(m_controlSettings.mouseSensitivity * 100));
    } else if (settingName == "Wall Run Sensitivity") {
        return std::to_string(static_cast<int>(m_parkourSettings.wallRunSensitivity * 100));
    } else if (settingName == "Jump Timing") {
        return std::to_string(static_cast<int>(m_parkourSettings.jumpTiming * 100));
    } else if (settingName == "Slide Control") {
        return std::to_string(static_cast<int>(m_parkourSettings.slideControl * 100));
    } else if (settingName == "Grapple Sensitivity") {
        return std::to_string(static_cast<int>(m_parkourSettings.grappleSensitivity * 100));
    } else if (settingName == "Difficulty") {
        auto& options = MenuConstants::DIFFICULTY_OPTIONS;
        if (m_gameplaySettings.difficultyLevel >= 0 && m_gameplaySettings.difficultyLevel < static_cast<int>(options.size())) {
            return options[m_gameplaySettings.difficultyLevel];
        }
    }

    return "Unknown";
}

// Audio settings getters
float SettingsManager::GetMasterVolume() const {
    return m_audioSettings.masterVolume;
}

float SettingsManager::GetMusicVolume() const {
    return m_audioSettings.musicVolume;
}

float SettingsManager::GetSfxVolume() const {
    return m_audioSettings.sfxVolume;
}

bool SettingsManager::IsMuted() const {
    return m_audioSettings.muted;
}

// Control settings getters
float SettingsManager::GetMouseSensitivity() const {
    return m_controlSettings.mouseSensitivity;
}

bool SettingsManager::GetInvertYAxis() const {
    return m_controlSettings.invertYAxis;
}

bool SettingsManager::GetControllerSupport() const {
    return m_controlSettings.controllerSupport;
}

// Parkour control settings getters
float SettingsManager::GetWallRunSensitivity() const {
    return m_parkourSettings.wallRunSensitivity;
}

float SettingsManager::GetJumpTiming() const {
    return m_parkourSettings.jumpTiming;
}

float SettingsManager::GetSlideControl() const {
    return m_parkourSettings.slideControl;
}

float SettingsManager::GetGrappleSensitivity() const {
    return m_parkourSettings.grappleSensitivity;
}

// Gameplay settings getters
int SettingsManager::GetDifficultyLevel() const {
    return m_gameplaySettings.difficultyLevel;
}

bool SettingsManager::IsTimerEnabled() const {
    return m_gameplaySettings.timerEnabled;
}

bool SettingsManager::AreCheckpointsEnabled() const {
    return m_gameplaySettings.checkpointsEnabled;
}

bool SettingsManager::IsAutoSaveEnabled() const {
    return m_gameplaySettings.autoSaveEnabled;
}

bool SettingsManager::IsSpeedrunMode() const {
    return m_gameplaySettings.speedrunMode;
}

bool SettingsManager::IsWallRunEnabled() const {
    return m_gameplaySettings.wallRunEnabled;
}

bool SettingsManager::IsDoubleJumpEnabled() const {
    return m_gameplaySettings.doubleJumpEnabled;
}

bool SettingsManager::IsSlideEnabled() const {
    return m_gameplaySettings.slideEnabled;
}

bool SettingsManager::IsGrappleEnabled() const {
    return m_gameplaySettings.grappleEnabled;
}

bool SettingsManager::IsSlowMotionOnTrick() const {
    return m_gameplaySettings.slowMotionOnTrick;
}

// Video settings getters
int SettingsManager::GetResolutionIndex() const {
    return m_currentResolutionIndex;
}

int SettingsManager::GetAspectRatioIndex() const {
    return m_currentAspectRatioIndex;
}

int SettingsManager::GetDisplayModeIndex() const {
    return m_currentDisplayModeIndex;
}

int SettingsManager::GetVSyncIndex() const {
    return m_currentVSyncIndex;
}

int SettingsManager::GetFpsIndex() const {
    return m_currentFpsIndex;
}

void SettingsManager::SetSkyboxGammaEnabled(bool enabled)
{
    m_config.SetSkyboxGammaEnabled(enabled);
}

bool SettingsManager::IsSkyboxGammaEnabled() const
{
    return m_config.IsSkyboxGammaEnabled();
}

void SettingsManager::SetSkyboxGammaValue(float gamma)
{
    // Clamp gamma value to reasonable range (0.5 to 3.0)
    float clampedGamma = std::max(0.5f, std::min(3.0f, gamma));
    m_config.SetSkyboxGammaValue(clampedGamma);
}

float SettingsManager::GetSkyboxGammaValue() const
{
    return m_config.GetSkyboxGammaValue();
}