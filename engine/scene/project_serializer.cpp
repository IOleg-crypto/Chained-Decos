#include "project_serializer.h"
#include "engine/core/log.h"
#include "fstream"
#include "yaml-cpp/yaml.h"

namespace CHEngine
{
ProjectSerializer::ProjectSerializer(std::shared_ptr<Project> project) : m_Project(project)
{
}

bool ProjectSerializer::Serialize(const std::filesystem::path &filepath)
{
    const auto &config = m_Project->m_Config;

    YAML::Emitter out;
    out << YAML::BeginMap; // Project
    out << YAML::Key << "Project" << YAML::Value;
    {
        out << YAML::BeginMap;
        out << YAML::Key << "Name" << YAML::Value << config.Name;
        out << YAML::Key << "StartScene" << YAML::Value << config.StartScene;
        out << YAML::Key << "AssetDirectory" << YAML::Value << config.AssetDirectory.string();
        out << YAML::Key << "ActiveScene" << YAML::Value << config.ActiveScenePath.string();
        out << YAML::Key << "Environment" << YAML::Value << config.EnvironmentPath.string();

        out << YAML::Key << "Physics" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Gravity" << YAML::Value << config.Physics.Gravity;
        out << YAML::EndMap;

        out << YAML::Key << "Animation" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "TargetFPS" << YAML::Value << config.Animation.TargetFPS;
        out << YAML::EndMap;

        out << YAML::Key << "Render" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "AmbientIntensity" << YAML::Value << config.Render.AmbientIntensity;
        out << YAML::Key << "DefaultExposure" << YAML::Value << config.Render.DefaultExposure;
        out << YAML::EndMap;

        out << YAML::Key << "Window" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Width" << YAML::Value << config.Window.Width;
        out << YAML::Key << "Height" << YAML::Value << config.Window.Height;
        out << YAML::Key << "VSync" << YAML::Value << config.Window.VSync;
        out << YAML::Key << "Resizable" << YAML::Value << config.Window.Resizable;
        out << YAML::EndMap;

        out << YAML::Key << "Runtime" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Fullscreen" << YAML::Value << config.Runtime.Fullscreen;
        out << YAML::Key << "ShowStats" << YAML::Value << config.Runtime.ShowStats;
        out << YAML::Key << "EnableConsole" << YAML::Value << config.Runtime.EnableConsole;
        out << YAML::EndMap;

        out << YAML::Key << "BuildConfig" << YAML::Value << (int)config.BuildConfig;

        out << YAML::EndMap;
    }
    out << YAML::EndMap; // Project

    std::ofstream fout(filepath);
    if (fout.is_open())
    {
        fout << out.c_str();
        return true;
    }

    CH_CORE_ERROR("Failed to save project file: {}", filepath.string());
    return false;
}

bool ProjectSerializer::Deserialize(const std::filesystem::path &filepath)
{
    std::ifstream stream(filepath);
    if (!stream.is_open())
    {
        CH_CORE_ERROR("Failed to open project file: {}", filepath.string());
        return false;
    }

    std::stringstream strStream;
    strStream << stream.rdbuf();

    YAML::Node data = YAML::Load(strStream.str());
    auto projectNode = data["Project"];
    if (!projectNode)
        return false;

    auto &config = m_Project->m_Config;
    config.Name = projectNode["Name"].as<std::string>();
    config.StartScene = projectNode["StartScene"].as<std::string>();
    config.AssetDirectory = projectNode["AssetDirectory"].as<std::string>();
    if (projectNode["Environment"])
        config.EnvironmentPath = projectNode["Environment"].as<std::string>();
    if (projectNode["ActiveScene"])
        config.ActiveScenePath = projectNode["ActiveScene"].as<std::string>();

    if (projectNode["Physics"])
        config.Physics.Gravity = projectNode["Physics"]["Gravity"].as<float>();

    if (projectNode["Animation"])
        config.Animation.TargetFPS = projectNode["Animation"]["TargetFPS"].as<float>();

    if (projectNode["Render"])
    {
        config.Render.AmbientIntensity = projectNode["Render"]["AmbientIntensity"].as<float>();
        config.Render.DefaultExposure = projectNode["Render"]["DefaultExposure"].as<float>();
    }

    if (projectNode["Window"])
    {
        config.Window.Width = projectNode["Window"]["Width"].as<int>();
        config.Window.Height = projectNode["Window"]["Height"].as<int>();
        config.Window.VSync = projectNode["Window"]["VSync"].as<bool>();
        config.Window.Resizable = projectNode["Window"]["Resizable"].as<bool>();
    }

    if (projectNode["Runtime"])
    {
        config.Runtime.Fullscreen = projectNode["Runtime"]["Fullscreen"].as<bool>();
        config.Runtime.ShowStats = projectNode["Runtime"]["ShowStats"].as<bool>();
        config.Runtime.EnableConsole = projectNode["Runtime"]["EnableConsole"].as<bool>();
    }

    if (projectNode["BuildConfig"])
        config.BuildConfig = (Configuration)projectNode["BuildConfig"].as<int>();

    config.ProjectDirectory = filepath.parent_path();

    return true;
}
} // namespace CHEngine
