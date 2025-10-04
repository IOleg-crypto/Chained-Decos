#include "ConfigManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_map>

bool ConfigManager::LoadFromFile(const std::string& filename)
{
    try
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            TraceLog(LOG_WARNING, "ConfigManager::LoadFromFile() - Could not open file: %s", filename.c_str());
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
                TraceLog(LOG_WARNING, "ConfigManager::LoadFromFile() - Invalid format on line %d: %s", lineNumber, line.c_str());
                continue;
            }

            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);

            Trim(key);
            Trim(value);

            if (!key.empty())
            {
                m_settings[ToLower(key)] = value;
                TraceLog(LOG_DEBUG, "ConfigManager::LoadFromFile() - Loaded setting: %s = %s", key.c_str(), value.c_str());
            }
        }

        file.close();
        TraceLog(LOG_INFO, "ConfigManager::LoadFromFile() - Successfully loaded %zu settings from %s", m_settings.size(), filename.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "ConfigManager::LoadFromFile() - Exception while loading %s: %s", filename.c_str(), e.what());
        return false;
    }
}

bool ConfigManager::SaveToFile(const std::string& filename)
{
    try
    {
        std::ofstream file(filename);
        if (!file.is_open())
        {
            TraceLog(LOG_ERROR, "ConfigManager::SaveToFile() - Could not open file for writing: %s", filename.c_str());
            return false;
        }

        file << "# Chained Decos Configuration File\n";
        file << "# Generated automatically - do not edit while game is running\n\n";

        for (const auto& pair : m_settings)
        {
            file << pair.first << " = " << pair.second << "\n";
        }

        file.close();
        TraceLog(LOG_INFO, "ConfigManager::SaveToFile() - Successfully saved %zu settings to %s", m_settings.size(), filename.c_str());
        return true;
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "ConfigManager::SaveToFile() - Exception while saving %s: %s", filename.c_str(), e.what());
        return false;
    }
}

// Simplified helper methods for cleaner code
int ConfigManager::GetInt(const std::string& key, int defaultValue) const
{
    auto it = m_settings.find(ToLower(key));
    if (it != m_settings.end())
    {
        try
        {
            return std::stoi(it->second);
        }
        catch (const std::exception&)
        {
            // Invalid integer value, return default
        }
    }
    return defaultValue;
}

float ConfigManager::GetFloat(const std::string& key, float defaultValue) const
{
    auto it = m_settings.find(ToLower(key));
    if (it != m_settings.end())
    {
        try
        {
            return std::stof(it->second);
        }
        catch (const std::exception&)
        {
            // Invalid float value, return default
        }
    }
    return defaultValue;
}

bool ConfigManager::GetBool(const std::string& key, bool defaultValue) const
{
    auto it = m_settings.find(ToLower(key));
    if (it != m_settings.end())
    {
        std::string value = ToLower(it->second);
        return value == "true" || value == "1" || value == "yes" || value == "on";
    }
    return defaultValue;
}

std::string ConfigManager::GetString(const std::string& key, const std::string& defaultValue) const
{
    auto it = m_settings.find(ToLower(key));
    return (it != m_settings.end()) ? it->second : defaultValue;
}

void ConfigManager::SetInt(const std::string& key, int value)
{
    m_settings[ToLower(key)] = std::to_string(value);
}

void ConfigManager::SetFloat(const std::string& key, float value)
{
    m_settings[ToLower(key)] = std::to_string(value);
}

void ConfigManager::SetBool(const std::string& key, bool value)
{
    m_settings[ToLower(key)] = value ? "true" : "false";
}

void ConfigManager::SetString(const std::string& key, const std::string& value)
{
    m_settings[ToLower(key)] = value;
}

void ConfigManager::SetResolution(int width, int height)
{
    SetInt("video_width", width);
    SetInt("video_height", height);
}

void ConfigManager::GetResolution(int& width, int& height) const
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

void ConfigManager::SetMouseSensitivity(float sensitivity)
{
    SetFloat("controls_sensitivity", sensitivity);
}

float ConfigManager::GetMouseSensitivity() const
{
    return GetFloat("controls_sensitivity", 1.0f);
}

void ConfigManager::SetInvertY(bool invert)
{
    SetBool("controls_invert_y", invert);
}

bool ConfigManager::GetInvertY() const
{
    return GetBool("controls_invert_y", false);
}

void ConfigManager::Trim(std::string& str) const
{
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        str = str.substr(start, end - start + 1);
    } else {
        str.clear();
    }
}

std::string ConfigManager::ToLower(const std::string& str) const
{
    std::string result = str;
    for (char& c : result) {
        c = std::tolower(c);
    }
    return result;
}

bool ConfigManager::IsCommentOrEmpty(const std::string& line) const
{
    return line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '/';
}