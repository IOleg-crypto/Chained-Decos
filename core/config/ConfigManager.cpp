#include "ConfigManager.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

bool ConfigManager::LoadFromFile(const std::string &filename)
{
    try
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            TraceLog(LOG_WARNING, "ConfigManager::LoadFromFile() - Could not open file: %s",
                     filename.c_str());
            return false;
        }

        std::string line;
        int lineNumber = 0;

        while (std::getline(file, line))
        {
            lineNumber++;
            Trim(line);

            if (IsCommentOrEmpty(line))
                continue;

            // Parse key-value pairs (format: key = value)
            size_t equalPos = line.find('=');
            if (equalPos == std::string::npos)
            {
                TraceLog(LOG_WARNING,
                         "ConfigManager::LoadFromFile() - Invalid format on line %d: %s",
                         lineNumber, line.c_str());
                continue;
            }

            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);

            Trim(key);
            Trim(value);

            if (!key.empty())
            {
                m_settings[ToLower(key)] = value;
            }
        }

        file.close();

        // Validate settings and set defaults for invalid values
        ValidateAndSetDefaults();

        return true;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "ConfigManager::LoadFromFile() - Exception while loading %s: %s",
                 filename.c_str(), e.what());
        return false;
    }
}

bool ConfigManager::SaveToFile(const std::string &filename)
{
    try
    {
        std::ofstream file(filename);
        if (!file.is_open())
        {
            TraceLog(LOG_ERROR, "ConfigManager::SaveToFile() - Could not open file for writing: %s",
                     filename.c_str());
            return false;
        }

        file << "# Chained Decos Configuration File\n";
        file << "# Generated automatically - do not edit while game is running\n\n";

        for (const auto &pair : m_settings)
        {
            file << pair.first << " = " << pair.second << "\n";
        }

        file.close();
        return true;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "ConfigManager::SaveToFile() - Exception while saving %s: %s",
                 filename.c_str(), e.what());
        return false;
    }
}

// Simplified helper methods for cleaner code
int ConfigManager::GetInt(const std::string &key, int defaultValue) const
{
    auto it = m_settings.find(ToLower(key));
    if (it != m_settings.end())
    {
        try
        {
            return std::stoi(it->second);
        }
        catch (const std::exception &)
        {
            // Invalid integer value, return default
        }
    }
    return defaultValue;
}

float ConfigManager::GetFloat(const std::string &key, float defaultValue) const
{
    auto it = m_settings.find(ToLower(key));
    if (it != m_settings.end())
    {
        try
        {
            return std::stof(it->second);
        }
        catch (const std::exception &)
        {
            // Invalid float value, return default
        }
    }
    return defaultValue;
}

bool ConfigManager::GetBool(const std::string &key, bool defaultValue) const
{
    auto it = m_settings.find(ToLower(key));
    if (it != m_settings.end())
    {
        std::string value = ToLower(it->second);
        return value == "true" || value == "1" || value == "yes" || value == "on";
    }
    return defaultValue;
}

std::string ConfigManager::GetString(const std::string &key, const std::string &defaultValue) const
{
    auto it = m_settings.find(ToLower(key));
    return (it != m_settings.end()) ? it->second : defaultValue;
}

void ConfigManager::SetInt(const std::string &key, int value)
{
    m_settings[ToLower(key)] = std::to_string(value);
}

void ConfigManager::SetFloat(const std::string &key, float value)
{
    m_settings[ToLower(key)] = std::to_string(value);
}

void ConfigManager::SetBool(const std::string &key, bool value)
{
    m_settings[ToLower(key)] = value ? "true" : "false";
}

void ConfigManager::SetString(const std::string &key, const std::string &value)
{
    m_settings[ToLower(key)] = value;
}

void ConfigManager::SetResolution(int width, int height)
{
    SetInt("video_width", width);
    SetInt("video_height", height);
}

void ConfigManager::GetResolution(int &width, int &height) const
{
    width = GetInt("video_width", 1280);
    height = GetInt("video_height", 720);
}

void ConfigManager::SetFullscreen(bool fullscreen)
{
    SetBool("video_fullscreen", fullscreen);
}

bool ConfigManager::IsFullscreen() const
{
    return GetBool("video_fullscreen", false);
}

void ConfigManager::SetVSync(bool vsync)
{
    SetBool("video_vsync", vsync);
}

bool ConfigManager::IsVSync() const
{
    return GetBool("video_vsync", true);
}

void ConfigManager::SetMasterVolume(float volume)
{
    SetFloat("audio_master", volume);
}

float ConfigManager::GetMasterVolume() const
{
    return GetFloat("audio_master", 1.0f);
}

void ConfigManager::SetMusicVolume(float volume)
{
    SetFloat("audio_music", volume);
}

float ConfigManager::GetMusicVolume() const
{
    return GetFloat("audio_music", 0.7f);
}

void ConfigManager::SetSFXVolume(float volume)
{
    SetFloat("audio_sfx", volume);
}

float ConfigManager::GetSFXVolume() const
{
    return GetFloat("audio_sfx", 0.8f);
}

float ConfigManager::GetMouseSensitivity() const
{
    return GetFloat("controls_sensitivity", 1.0f);
}

void ConfigManager::SetMouseSensitivity(float sensitivity)
{
    SetFloat("controls_sensitivity", sensitivity);
}

void ConfigManager::SetInvertY(bool invert)
{
    SetBool("controls_invert_y", invert);
}

bool ConfigManager::GetInvertY() const
{
    return GetBool("controls_invert_y", false);
}

// Parkour-specific control settings
void ConfigManager::SetWallRunSensitivity(float sensitivity)
{
    SetFloat("parkour_wallrun_sensitivity", sensitivity);
}

float ConfigManager::GetWallRunSensitivity() const
{
    return GetFloat("parkour_wallrun_sensitivity", 1.0f);
}

void ConfigManager::SetJumpTiming(float timing)
{
    SetFloat("parkour_jump_timing", timing);
}

float ConfigManager::GetJumpTiming() const
{
    return GetFloat("parkour_jump_timing", 1.0f);
}

void ConfigManager::SetSlideControl(float control)
{
    SetFloat("parkour_slide_control", control);
}

float ConfigManager::GetSlideControl() const
{
    return GetFloat("parkour_slide_control", 1.0f);
}

void ConfigManager::SetGrappleSensitivity(float sensitivity)
{
    SetFloat("parkour_grapple_sensitivity", sensitivity);
}

float ConfigManager::GetGrappleSensitivity() const
{
    return GetFloat("parkour_grapple_sensitivity", 1.0f);
}

void ConfigManager::SetTimerEnabled(bool enabled)
{
    SetBool("gameplay_timer_enabled", enabled);
}

bool ConfigManager::IsTimerEnabled() const
{
    return GetBool("gameplay_timer_enabled", true);
}

void ConfigManager::SetCheckpointsEnabled(bool enabled)
{
    SetBool("gameplay_checkpoints_enabled", enabled);
}

bool ConfigManager::AreCheckpointsEnabled() const
{
    return GetBool("gameplay_checkpoints_enabled", true);
}

bool ConfigManager::IsAutoSaveEnabled() const
{
    return GetBool("gameplay_autosave_enabled", true);
}

void ConfigManager::SetAutoSaveEnabled(bool enabled)
{
    SetBool("gameplay_autosave_enabled", enabled);
}

void ConfigManager::SetDifficultyLevel(int level)
{
    SetInt("gameplay_difficulty", level);
}

int ConfigManager::GetDifficultyLevel() const
{
    return GetInt("gameplay_difficulty", 1);
}

void ConfigManager::SetSpeedrunMode(bool enabled)
{
    SetBool("gameplay_speedrun_mode", enabled);
}

bool ConfigManager::IsSpeedrunMode() const
{
    return GetBool("gameplay_speedrun_mode", false);
}

// Graphics quality settings for parkour
void ConfigManager::SetShadowQuality(int quality)
{
    SetInt("graphics_shadow_quality", quality);
}

int ConfigManager::GetShadowQuality() const
{
    return GetInt("graphics_shadow_quality", 2); // Default to medium
}

void ConfigManager::SetAntiAliasing(int level)
{
    SetInt("graphics_antialiasing", level);
}

int ConfigManager::GetAntiAliasing() const
{
    return GetInt("graphics_antialiasing", 2); // Default to medium
}

void ConfigManager::SetTextureQuality(int quality)
{
    SetInt("graphics_texture_quality", quality);
}

int ConfigManager::GetTextureQuality() const
{
    return GetInt("graphics_texture_quality", 2); // Default to medium
}

void ConfigManager::SetRenderDistance(float distance)
{
    SetFloat("graphics_render_distance", distance);
}

float ConfigManager::GetRenderDistance() const
{
    return GetFloat("graphics_render_distance", 100.0f);
}

void ConfigManager::SetSkyboxGammaEnabled(bool enabled)
{
    SetBool("skybox_gamma_enabled", enabled);
}

bool ConfigManager::IsSkyboxGammaEnabled() const
{
    return GetBool("skybox_gamma_enabled", false);
}

void ConfigManager::SetSkyboxGammaValue(float gamma)
{
    SetFloat("skybox_gamma_value", gamma);
}

float ConfigManager::GetSkyboxGammaValue() const
{
    return GetFloat("skybox_gamma_value", 2.2f);
}

// Game progression settings
void ConfigManager::SetCompletedLevels(const std::string &levels)
{
    SetString("progression_completed_levels", levels);
}

std::string ConfigManager::GetCompletedLevels() const
{
    return GetString("progression_completed_levels", "");
}

void ConfigManager::SetBestTimes(const std::string &times)
{
    SetString("progression_best_times", times);
}

std::string ConfigManager::GetBestTimes() const
{
    return GetString("progression_best_times", "");
}

void ConfigManager::SetTotalPlayTime(float time)
{
    SetFloat("progression_total_playtime", time);
}

float ConfigManager::GetTotalPlayTime() const
{
    return GetFloat("progression_total_playtime", 0.0f);
}

bool ConfigManager::IsDoubleJumpEnabled() const
{
    return GetBool("parkour_doublejump_enabled", false);
}

void ConfigManager::SetDoubleJumpEnabled(bool enabled)
{
    SetBool("parkour_doublejump_enabled", enabled);
}

void ConfigManager::SetWallRunEnabled(bool enabled)
{
    SetBool("parkour_wallrun_enabled", enabled);
}

bool ConfigManager::IsWallRunEnabled() const
{
    return GetBool("parkour_wallrun_enabled", true);
}

void ConfigManager::SetSlideEnabled(bool enabled)
{
    SetBool("parkour_slide_enabled", enabled);
}

bool ConfigManager::IsSlideEnabled() const
{
    return GetBool("parkour_slide_enabled", true);
}

void ConfigManager::SetGrappleEnabled(bool enabled)
{
    SetBool("parkour_grapple_enabled", enabled);
}

bool ConfigManager::IsGrappleEnabled() const
{
    return GetBool("parkour_grapple_enabled", true);
}

void ConfigManager::SetSlowMotionOnTrick(bool enabled)
{
    SetBool("parkour_slowmotion_enabled", enabled);
}

bool ConfigManager::IsSlowMotionOnTrick() const
{
    return GetBool("parkour_slowmotion_enabled", true);
}

// Settings validation and default value management
void ConfigManager::ValidateAndSetDefaults()
{
    // Video settings validation
    if (GetInt("video_width", 0) < 800)
        SetInt("video_width", 1280);
    if (GetInt("video_height", 0) < 600)
        SetInt("video_height", 720);
    if (GetFloat("audio_master", -1.0f) < 0.0f || GetFloat("audio_master", 2.0f) > 1.0f)
        SetFloat("audio_master", 1.0f);
    if (GetFloat("audio_music", -1.0f) < 0.0f || GetFloat("audio_music", 2.0f) > 1.0f)
        SetFloat("audio_music", 0.7f);
    if (GetFloat("audio_sfx", -1.0f) < 0.0f || GetFloat("audio_sfx", 2.0f) > 1.0f)
        SetFloat("audio_sfx", 0.8f);

    // Control settings validation
    if (GetFloat("controls_sensitivity", 0.0f) < 0.1f ||
        GetFloat("controls_sensitivity", 10.0f) > 5.0f)
        SetFloat("controls_sensitivity", 1.0f);

    // Graphics settings validation
    if (GetInt("graphics_shadow_quality", 0) < 1 || GetInt("graphics_shadow_quality", 4) > 3)
        SetInt("graphics_shadow_quality", 2);
    if (GetInt("graphics_antialiasing", 0) < 1 || GetInt("graphics_antialiasing", 5) > 4)
        SetInt("graphics_antialiasing", 2);
    if (GetInt("graphics_texture_quality", 0) < 1 || GetInt("graphics_texture_quality", 4) > 3)
        SetInt("graphics_texture_quality", 2);
}

void ConfigManager::Trim(std::string &str) const
{
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos)
    {
        str = str.substr(start, end - start + 1);
    }
    else
    {
        str.clear();
    }
}

std::string ConfigManager::ToLower(const std::string &str) const
{
    std::string result = str;
    for (char &c : result)
    {
        c = std::tolower(c);
    }
    return result;
}

bool ConfigManager::IsCommentOrEmpty(const std::string &line) const
{
    return line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '/';
}



