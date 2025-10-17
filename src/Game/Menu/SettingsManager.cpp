#include "SettingsManager.h"
#include <raylib.h>
#include <iostream>

SettingsManager::SettingsManager() {
    LoadSettings();
}

void SettingsManager::LoadSettings() {
    // Load configuration from file (try current directory first, then build directory)
    try {
        if (!config.LoadFromFile("game.cfg")) {
            // Try loading from build directory if not found in current directory
            if (!config.LoadFromFile("build/game.cfg")) {
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
    config.GetResolution(width, height);
    SetWindowSize(width, height);

    if (config.IsFullscreen()) {
        SetWindowState(FLAG_FULLSCREEN_MODE);
    }

    if (config.IsVSync()) {
        SetWindowState(FLAG_VSYNC_HINT);
    }

    // Load audio settings
    m_audioSettings.masterVolume = m_config.GetMasterVolume();
    m_audioSettings.musicVolume = m_config.GetMusicVolume();
    m_audioSettings.sfxVolume = m_config.GetSfxVolume();
    m_audioSettings.muted = m_config.IsAudioMuted();

    // Load control settings
    m_controlSettings.mouseSensitivity = m_config.GetMouseSensitivity();
    m_controlSettings.invertYAxis = m_config.IsYAxisInverted();
    m_controlSettings.controllerSupport = m_config.IsControllerSupportEnabled();

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
    currentResolutionIndex = 1; // Default to 1280x720
    currentAspectRatioIndex = 0; // Default to 16:9
    currentDisplayModeIndex = config.IsFullscreen() ? 1 : 0;
    currentVSyncIndex = config.IsVSync() ? 1 : 0;
    currentFpsIndex = 1; // Default to 60 FPS
}

void SettingsManager::SaveSettings() {
    try {
        // Save current window settings
        config.SetResolution(GetScreenWidth(), GetScreenHeight());
        config.SetFullscreen(IsWindowFullscreen());
        config.SetVSync(IsWindowState(FLAG_VSYNC_HINT));

        // Save audio settings
        config.SetMasterVolume(audioSettings.masterVolume);
        config.SetMusicVolume(audioSettings.musicVolume);
        config.SetSfxVolume(audioSettings.sfxVolume);
        config.SetAudioMuted(audioSettings.muted);

        // Save control settings
        config.SetMouseSensitivity(controlSettings.mouseSensitivity);
        config.SetYAxisInverted(controlSettings.invertYAxis);
        config.SetControllerSupportEnabled(controlSettings.controllerSupport);

        // Save parkour-specific settings
        config.SetWallRunSensitivity(parkourSettings.wallRunSensitivity);
        config.SetJumpTiming(parkourSettings.jumpTiming);
        config.SetSlideControl(parkourSettings.slideControl);
        config.SetGrappleSensitivity(parkourSettings.grappleSensitivity);

        // Save gameplay settings
        config.SetDifficultyLevel(gameplaySettings.difficultyLevel);
        config.SetTimerEnabled(gameplaySettings.timerEnabled);
        config.SetCheckpointsEnabled(gameplaySettings.checkpointsEnabled);
        config.SetAutoSaveEnabled(gameplaySettings.autoSaveEnabled);
        config.SetSpeedrunMode(gameplaySettings.speedrunMode);

        config.SetWallRunEnabled(gameplaySettings.wallRunEnabled);
        config.SetDoubleJumpEnabled(gameplaySettings.doubleJumpEnabled);
        config.SetSlideEnabled(gameplaySettings.slideEnabled);
        config.SetGrappleEnabled(gameplaySettings.grappleEnabled);
        config.SetSlowMotionOnTrick(gameplaySettings.slowMotionOnTrick);

        // Save to file (try current directory first, then build directory)
        if (!config.SaveToFile("game.cfg")) {
            // Try saving to build directory if not found in current directory
            if (!config.SaveToFile("build/game.cfg")) {
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
    auto& resolutionOptions = RESOLUTION_OPTIONS;
    if (currentResolutionIndex >= 0 && currentResolutionIndex < static_cast<int>(resolutionOptions.size())) {
        auto resolution = resolutionOptions[currentResolutionIndex];
        // Parse resolution string (e.g., "1920x1080")
        size_t xPos = resolution.find('x');
        if (xPos != std::string::npos) {
            int width = std::stoi(resolution.substr(0, xPos));
            int height = std::stoi(resolution.substr(xPos + 1));
            SetWindowSize(width, height);
        }
    }

    // Apply fullscreen mode
    if (currentDisplayModeIndex == 1) { // Fullscreen
        SetWindowState(FLAG_FULLSCREEN_MODE);
    } else if (currentDisplayModeIndex == 2) { // Borderless
        // Borderless implementation would go here
    } else { // Windowed
        ClearWindowState(FLAG_FULLSCREEN_MODE);
    }

    // Apply VSync
    if (currentVSyncIndex == 1) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }

    // Apply FPS target
    auto& fpsOptions = FPS_OPTIONS;
    if (currentFpsIndex >= 0 && currentFpsIndex < static_cast<int>(fpsOptions.size())) {
        auto fps = fpsOptions[currentFpsIndex];
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
    audioSettings.masterVolume = std::max(0.0f, std::min(1.0f, volume));
}

void SettingsManager::SetMusicVolume(float volume) {
    audioSettings.musicVolume = std::max(0.0f, std::min(1.0f, volume));
}

void SettingsManager::SetSfxVolume(float volume) {
    audioSettings.sfxVolume = std::max(0.0f, std::min(1.0f, volume));
}

void SettingsManager::SetMuted(bool muted) {
    audioSettings.muted = muted;
}

// Control settings methods
void SettingsManager::SetMouseSensitivity(float sensitivity) {
    controlSettings.mouseSensitivity = std::max(0.1f, std::min(5.0f, sensitivity));
}

void SettingsManager::SetInvertYAxis(bool invert) {
    controlSettings.invertYAxis = invert;
}

void SettingsManager::SetControllerSupport(bool enabled) {
    controlSettings.controllerSupport = enabled;
}

// Parkour control settings methods
void SettingsManager::SetWallRunSensitivity(float sensitivity) {
    parkourSettings.wallRunSensitivity = std::max(0.1f, std::min(5.0f, sensitivity));
}

void SettingsManager::SetJumpTiming(float timing) {
    parkourSettings.jumpTiming = std::max(0.1f, std::min(5.0f, timing));
}

void SettingsManager::SetSlideControl(float control) {
    parkourSettings.slideControl = std::max(0.1f, std::min(5.0f, control));
}

void SettingsManager::SetGrappleSensitivity(float sensitivity) {
    parkourSettings.grappleSensitivity = std::max(0.1f, std::min(5.0f, sensitivity));
}

// Gameplay settings methods
void SettingsManager::SetDifficultyLevel(int level) {
    gameplaySettings.difficultyLevel = std::max(0, std::min(2, level));
}

void SettingsManager::SetTimerEnabled(bool enabled) {
    gameplaySettings.timerEnabled = enabled;
}

void SettingsManager::SetCheckpointsEnabled(bool enabled) {
    gameplaySettings.checkpointsEnabled = enabled;
}

void SettingsManager::SetAutoSaveEnabled(bool enabled) {
    gameplaySettings.autoSaveEnabled = enabled;
}

void SettingsManager::SetSpeedrunMode(bool enabled) {
    gameplaySettings.speedrunMode = enabled;
}

void SettingsManager::SetWallRunEnabled(bool enabled) {
    gameplaySettings.wallRunEnabled = enabled;
}

void SettingsManager::SetDoubleJumpEnabled(bool enabled) {
    gameplaySettings.doubleJumpEnabled = enabled;
}

void SettingsManager::SetSlideEnabled(bool enabled) {
    gameplaySettings.slideEnabled = enabled;
}

void SettingsManager::SetGrappleEnabled(bool enabled) {
    gameplaySettings.grappleEnabled = enabled;
}

void SettingsManager::SetSlowMotionOnTrick(bool enabled) {
    gameplaySettings.slowMotionOnTrick = enabled;
}

// Video settings methods
void SettingsManager::SetResolutionIndex(int index) {
    currentResolutionIndex = std::max(0, std::min(static_cast<int>(RESOLUTION_OPTIONS.size()) - 1, index));
}

void SettingsManager::SetAspectRatioIndex(int index) {
    currentAspectRatioIndex = std::max(0, std::min(static_cast<int>(ASPECT_RATIO_OPTIONS.size()) - 1, index));
}

void SettingsManager::SetDisplayModeIndex(int index) {
    currentDisplayModeIndex = std::max(0, std::min(static_cast<int>(DISPLAY_MODE_OPTIONS.size()) - 1, index));
}

void SettingsManager::SetVSyncIndex(int index) {
    currentVSyncIndex = std::max(0, std::min(static_cast<int>(VSYNC_OPTIONS.size()) - 1, index));
}

void SettingsManager::SetFpsIndex(int index) {
    currentFpsIndex = std::max(0, std::min(static_cast<int>(FPS_OPTIONS.size()) - 1, index));
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
        auto& options = DIFFICULTY_OPTIONS;
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