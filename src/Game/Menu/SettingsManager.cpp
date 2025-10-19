#include "SettingsManager.h"
#include "MenuConstants.h"
#include <raylib.h>
#include <iostream>
#include <algorithm>

SettingsManager::SettingsManager() {
    LoadSettings();
}

void SettingsManager::LoadSettings() {
    // Load configuration from file (try current directory first, then build directory)
    try {
        if (!m_config.LoadFromFile("game.cfg")) {
            // Try loading from build directory if not found in current directory
            if (!m_config.LoadFromFile("build/game.cfg")) {
                TraceLog(LOG_WARNING, "SettingsManager::LoadSettings() - Could not load game.cfg, will use default settings");
            }
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "SettingsManager::LoadSettings() - Exception while loading configuration: %s", e.what());
        TraceLog(LOG_INFO, "SettingsManager::LoadSettings() - Continuing with default settings");
    } catch (...) {
        TraceLog(LOG_ERROR, "SettingsManager::LoadSettings() - Unknown exception while loading configuration");
        TraceLog(LOG_INFO, "SettingsManager::LoadSettings() - Continuing with default settings");
    }

    // Apply loaded settings to the game
    int width, height;
    m_config.GetResolution(width, height);
    SetWindowSize(width, height);

    if (m_config.IsFullscreen()) {
        SetWindowState(FLAG_FULLSCREEN_MODE);
    }

    if (m_config.IsVSync()) {
        SetWindowState(FLAG_VSYNC_HINT);
    }

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
    try {
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

        // Save to file (try current directory first, then build directory)
        if (!m_config.SaveToFile("game.cfg")) {
            // Try saving to build directory if not found in current directory
            if (!m_config.SaveToFile("build/game.cfg")) {
                TraceLog(LOG_WARNING, "SettingsManager::SaveSettings() - Could not save game.cfg");
            } else {
                TraceLog(LOG_INFO, "SettingsManager::SaveSettings() - Settings saved to build/game.cfg");
            }
        } else {
            TraceLog(LOG_INFO, "SettingsManager::SaveSettings() - Settings saved to game.cfg");
        }
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "SettingsManager::SaveSettings() - Exception while saving settings: %s", e.what());
    } catch (...) {
        TraceLog(LOG_ERROR, "SettingsManager::SaveSettings() - Unknown exception while saving settings");
    }
}

void SettingsManager::ApplyVideoSettings() {
    // Get resolution from options
    auto& resolutionOptions = MenuConstants::RESOLUTION_OPTIONS;
    if (m_currentResolutionIndex >= 0 && m_currentResolutionIndex < static_cast<int>(resolutionOptions.size())) {
        auto resolution = resolutionOptions[m_currentResolutionIndex];
        // Parse resolution string (e.g., "1920x1080")
        size_t xPos = resolution.find('x');
        if (xPos != std::string::npos) {
            int width = std::stoi(resolution.substr(0, xPos));
            int height = std::stoi(resolution.substr(xPos + 1));
            SetWindowSize(width, height);
        }
    }

    // Apply fullscreen mode
    if (m_currentDisplayModeIndex == 1) { // Fullscreen
        SetWindowState(FLAG_FULLSCREEN_MODE);
    } else if (m_currentDisplayModeIndex == 2) { // Borderless
        // Borderless implementation would go here
    } else { // Windowed
        ClearWindowState(FLAG_FULLSCREEN_MODE);
    }

    // Apply VSync
    if (m_currentVSyncIndex == 1) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }

    // Apply FPS target
    auto& fpsOptions = MenuConstants::FPS_OPTIONS;
    if (m_currentFpsIndex >= 0 && m_currentFpsIndex < static_cast<int>(fpsOptions.size())) {
        auto fps = fpsOptions[m_currentFpsIndex];
        if (fps == "Unlimited") {
            SetTargetFPS(0);
        } else {
            SetTargetFPS(std::stoi(fps));
        }
    }
}

void SettingsManager::ApplyAudioSettings() {
    // Apply audio settings to the audio system
    // This would integrate with your audio manager
    TraceLog(LOG_INFO, "SettingsManager::ApplyAudioSettings() - Applying audio settings");
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