#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>
#include <unordered_map>
#include <raylib.h>

class ConfigManager
{
public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    // Load configuration from file
    bool LoadFromFile(const std::string& filename);

    // Save configuration to file
    bool SaveToFile(const std::string& filename);

    // Getters for common settings
    int GetInt(const std::string& key, int defaultValue = 0) const;
    float GetFloat(const std::string& key, float defaultValue = 0.0f) const;
    bool GetBool(const std::string& key, bool defaultValue = false) const;
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;

    // Setters for common settings
    void SetInt(const std::string& key, int value);
    void SetFloat(const std::string& key, float value);
    void SetBool(const std::string& key, bool value);
    void SetString(const std::string& key, const std::string& value);

    // Video settings
    void SetResolution(int width, int height);
    void GetResolution(int& width, int& height) const;
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

private:
    std::unordered_map<std::string, std::string> m_settings;

    // Helper methods
    void Trim(std::string& str) const;
    std::string ToLower(const std::string& str) const;
    bool IsCommentOrEmpty(const std::string& line) const;
};

#endif // CONFIGMANAGER_H