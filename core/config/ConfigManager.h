#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <raylib.h>
#include <string>
#include <unordered_map>

class ConfigManager
{
public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    // Load configuration from file
    bool LoadFromFile(const std::string &filename);

    // Save configuration to file
    bool SaveToFile(const std::string &filename);

    // Getters for common settings
    int GetInt(const std::string &key, int defaultValue = 0) const;
    float GetFloat(const std::string &key, float defaultValue = 0.0f) const;
    bool GetBool(const std::string &key, bool defaultValue = false) const;
    std::string GetString(const std::string &key, const std::string &defaultValue = "") const;

    // Setters for common settings
    void SetInt(const std::string &key, int value);
    void SetFloat(const std::string &key, float value);
    void SetBool(const std::string &key, bool value);
    void SetString(const std::string &key, const std::string &value);

    // Video settings
    void SetResolution(int width, int height);
    void GetResolution(int &width, int &height) const;
    void SetFullscreen(bool fullscreen);
    bool IsFullscreen() const;
    void SetVSync(bool vsync);
    bool IsVSync() const;

    // Audio settings
    void SetMasterVolume(float volume);
    float GetMasterVolume() const;
    void SetMusicVolume(float volume);
    float GetMusicVolume() const;
    void SetSFXVolume(float volume);
    float GetSFXVolume() const;

    // Control settings
    void SetMouseSensitivity(float sensitivity);
    float GetMouseSensitivity() const;
    void SetInvertY(bool invert);
    bool GetInvertY() const;

    // Parkour-specific control settings
    void SetWallRunSensitivity(float sensitivity);
    float GetWallRunSensitivity() const;
    void SetJumpTiming(float timing);
    float GetJumpTiming() const;
    void SetSlideControl(float control);
    float GetSlideControl() const;
    void SetGrappleSensitivity(float sensitivity);
    float GetGrappleSensitivity() const;

    // Gameplay settings
    void SetDifficultyLevel(int level);
    int GetDifficultyLevel() const;
    void SetTimerEnabled(bool enabled);
    bool IsTimerEnabled() const;
    void SetCheckpointsEnabled(bool enabled);
    bool AreCheckpointsEnabled() const;
    void SetAutoSaveEnabled(bool enabled);
    bool IsAutoSaveEnabled() const;
    void SetSpeedrunMode(bool enabled);
    bool IsSpeedrunMode() const;

    // Graphics quality settings for parkour
    void SetShadowQuality(int quality);
    int GetShadowQuality() const;
    void SetAntiAliasing(int level);
    int GetAntiAliasing() const;
    void SetTextureQuality(int quality);
    int GetTextureQuality() const;
    void SetRenderDistance(float distance);
    float GetRenderDistance() const;

    // Skybox gamma settings
    void SetSkyboxGammaEnabled(bool enabled);
    bool IsSkyboxGammaEnabled() const;
    void SetSkyboxGammaValue(float gamma);
    float GetSkyboxGammaValue() const;
    void SetSkyboxExposure(float exposure);
    float GetSkyboxExposure() const;
    void SetSkyboxBrightness(float brightness);
    float GetSkyboxBrightness() const;
    void SetSkyboxContrast(float contrast);
    float GetSkyboxContrast() const;

    // Runtime settings
    void SetDefaultScenePath(const std::string &path);
    std::string GetDefaultScenePath() const;

    // Game progression settings
    void SetCompletedLevels(const std::string &levels);
    std::string GetCompletedLevels() const;
    void SetUnlockedMaps(const std::string &maps);
    std::string GetUnlockedMaps() const;
    void SetBestTimes(const std::string &times);
    std::string GetBestTimes() const;
    void SetTotalPlayTime(float time);
    float GetTotalPlayTime() const;

    // Advanced parkour settings
    void SetWallRunEnabled(bool enabled);
    bool IsWallRunEnabled() const;
    void SetDoubleJumpEnabled(bool enabled);
    bool IsDoubleJumpEnabled() const;
    void SetSlideEnabled(bool enabled);
    bool IsSlideEnabled() const;
    void SetGrappleEnabled(bool enabled);
    bool IsGrappleEnabled() const;
    void SetSlowMotionOnTrick(bool enabled);
    bool IsSlowMotionOnTrick() const;

    // Settings validation and default value management
    void ValidateAndSetDefaults();

private:
    std::unordered_map<std::string, std::string> m_settings;

    // Helper methods
    void Trim(std::string &str) const;
    std::string ToLower(const std::string &str) const;
    bool IsCommentOrEmpty(const std::string &line) const;
};

#endif // CONFIGMANAGER_H
