/**
 * @file SettingsManager.h
 * @brief Centralized settings management for the Menu system
 *
 * The SettingsManager class handles all game settings including audio, video,
 * controls, and gameplay preferences. It provides a clean interface for loading,
 * saving, and applying settings throughout the application.
 */

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <string>
#include "../Config/ConfigManager.h"
#include "MenuConstants.h"

namespace MenuConstants {
    using namespace MenuConstants;
}

/**
 * @struct AudioSettings
 * @brief Container for audio-related settings
 */
struct AudioSettings {
    float masterVolume = DEFAULT_MASTER_VOLUME;
    float musicVolume = DEFAULT_MUSIC_VOLUME;
    float sfxVolume = DEFAULT_SFX_VOLUME;
    bool muted = false;
};

struct ControlSettings {
    float mouseSensitivity = DEFAULT_MOUSE_SENSITIVITY;
    bool invertYAxis = false;
    bool controllerSupport = true;
};

struct ParkourControlSettings {
    float wallRunSensitivity = DEFAULT_WALL_RUN_SENSITIVITY;
    float jumpTiming = DEFAULT_JUMP_TIMING;
    float slideControl = DEFAULT_SLIDE_CONTROL;
    float grappleSensitivity = DEFAULT_GRAPPLE_SENSITIVITY;
};

struct GameplaySettings {
    int difficultyLevel = DEFAULT_DIFFICULTY_LEVEL;
    bool timerEnabled = true;
    bool checkpointsEnabled = true;
    bool autoSaveEnabled = true;
    bool speedrunMode = false;

    // Advanced parkour settings
    bool wallRunEnabled = true;
    bool doubleJumpEnabled = false;
    bool slideEnabled = true;
    bool grappleEnabled = false;
    bool slowMotionOnTrick = false;
};

/**
 * @class SettingsManager
 * @brief Manages all game settings with validation and persistence
 *
 * This class provides a centralized interface for handling audio, video,
 * control, and gameplay settings. It includes validation, persistence
 * to configuration files, and application of settings to the game engine.
 */
class SettingsManager {
private:
    ConfigManager m_config;
    AudioSettings m_audioSettings;
    ControlSettings m_controlSettings;
    ParkourControlSettings m_parkourSettings;
    GameplaySettings m_gameplaySettings;

    // Video settings (stored in ConfigManager)
    int m_currentResolutionIndex = 1; // Default to 1280x720
    int m_currentAspectRatioIndex = 0; // Default to 16:9
    int m_currentDisplayModeIndex = 0; // Default to Windowed
    int m_currentVSyncIndex = 1; // Default to On
    int m_currentFpsIndex = 1; // Default to 60 FPS

public:
    /**
     * @brief Default constructor that loads settings from configuration file
     */
    SettingsManager();

    // Load and save settings
    void LoadSettings();
    void SaveSettings();

    // Audio settings
    void SetMasterVolume(float volume);
    void SetMusicVolume(float volume);
    void SetSfxVolume(float volume);
    void SetMuted(bool muted);

    float GetMasterVolume() const;
    float GetMusicVolume() const;
    float GetSfxVolume() const;
    bool IsMuted() const;

    // Control settings
    void SetMouseSensitivity(float sensitivity);
    void SetInvertYAxis(bool invert);
    void SetControllerSupport(bool enabled);

    float GetMouseSensitivity() const;
    bool GetInvertYAxis() const;
    bool GetControllerSupport() const;

    // Parkour control settings
    void SetWallRunSensitivity(float sensitivity);
    void SetJumpTiming(float timing);
    void SetSlideControl(float control);
    void SetGrappleSensitivity(float sensitivity);

    float GetWallRunSensitivity() const;
    float GetJumpTiming() const;
    float GetSlideControl() const;
    float GetGrappleSensitivity() const;

    // Gameplay settings
    void SetDifficultyLevel(int level);
    void SetTimerEnabled(bool enabled);
    void SetCheckpointsEnabled(bool enabled);
    void SetAutoSaveEnabled(bool enabled);
    void SetSpeedrunMode(bool enabled);

    void SetWallRunEnabled(bool enabled);
    void SetDoubleJumpEnabled(bool enabled);
    void SetSlideEnabled(bool enabled);
    void SetGrappleEnabled(bool enabled);
    void SetSlowMotionOnTrick(bool enabled);

    int GetDifficultyLevel() const;
    bool IsTimerEnabled() const;
    bool AreCheckpointsEnabled() const;
    bool IsAutoSaveEnabled() const;
    bool IsSpeedrunMode() const;

    bool IsWallRunEnabled() const;
    bool IsDoubleJumpEnabled() const;
    bool IsSlideEnabled() const;
    bool IsGrappleEnabled() const;
    bool IsSlowMotionOnTrick() const;

    // Video settings
    void SetResolutionIndex(int index);
    void SetAspectRatioIndex(int index);
    void SetDisplayModeIndex(int index);
    void SetVSyncIndex(int index);
    void SetFpsIndex(int index);

    int GetResolutionIndex() const;
    int GetAspectRatioIndex() const;
    int GetDisplayModeIndex() const;
    int GetVSyncIndex() const;
    int GetFpsIndex() const;

    // Apply settings to the system
    void ApplyVideoSettings();
    void ApplyAudioSettings();

    // Get current setting values as strings
    std::string GetCurrentSettingValue(const std::string& settingName) const;

private:
    // Validation helpers
    float ClampVolume(float volume) const { return std::max(0.0f, std::min(1.0f, volume)); }
    float ClampSensitivity(float sensitivity) const { return std::max(0.1f, std::min(5.0f, sensitivity)); }
    int ClampDifficulty(int level) const { return std::max(0, std::min(2, level)); }
    int ClampOptionIndex(int index, int maxValue) const { return std::max(0, std::min(maxValue - 1, index)); }

    // Internal helpers
    void ResetToDefaults();
    bool ValidateSettings() const;
};

#endif // SETTINGS_MANAGER_H