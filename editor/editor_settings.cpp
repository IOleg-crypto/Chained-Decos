#include "editor_settings.h"
#include "engine/core/log.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{
EditorSettingsData EditorSettings::s_Data;

void EditorSettings::Init()
{
    Load();
}

void EditorSettings::Shutdown()
{
    Save();
}

void EditorSettings::Load()
{
    auto path = GetConfigPath();
    if (!std::filesystem::exists(path))
        return;

    try
    {
        YAML::Node config = YAML::LoadFile(path.string());
        if (config["Editor"])
        {
            auto node = config["Editor"];
            if (node["WindowWidth"])
                s_Data.WindowWidth = node["WindowWidth"].as<int>();
            if (node["WindowHeight"])
                s_Data.WindowHeight = node["WindowHeight"].as<int>();
            if (node["Fullscreen"])
                s_Data.Fullscreen = node["Fullscreen"].as<bool>();
            if (node["TargetFPS"])
                s_Data.TargetFPS = node["TargetFPS"].as<int>();
            if (node["VSync"])
                s_Data.VSync = node["VSync"].as<bool>();
            if (node["LastProjectPath"])
                s_Data.LastProjectPath = node["LastProjectPath"].as<std::string>();
        }
    }
    catch (const std::exception &e)
    {
        CH_CORE_ERROR("Failed to load editor settings: {0}", e.what());
    }
}

void EditorSettings::Save()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Editor" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "WindowWidth" << YAML::Value << s_Data.WindowWidth;
    out << YAML::Key << "WindowHeight" << YAML::Value << s_Data.WindowHeight;
    out << YAML::Key << "Fullscreen" << YAML::Value << s_Data.Fullscreen;
    out << YAML::Key << "TargetFPS" << YAML::Value << s_Data.TargetFPS;
    out << YAML::Key << "VSync" << YAML::Value << s_Data.VSync;
    out << YAML::Key << "LastProjectPath" << YAML::Value << s_Data.LastProjectPath;
    out << YAML::EndMap;
    out << YAML::EndMap;

    std::ofstream fout(GetConfigPath());
    fout << out.c_str();
}

std::filesystem::path EditorSettings::GetConfigPath()
{
    return "editor_settings.yaml";
}

} // namespace CHEngine
