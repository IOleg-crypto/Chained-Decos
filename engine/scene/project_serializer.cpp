#include "project_serializer.h"
#include "engine/core/log.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{
ProjectSerializer::ProjectSerializer(Ref<Project> project) : m_Project(project)
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
        out << YAML::EndMap;
    }
    out << YAML::EndMap; // Project

    std::ofstream fout(filepath);
    if (fout.is_open())
    {
        fout << out.c_str();
        return true;
    }

    CH_CORE_ERROR("Failed to save project file: {0}", filepath.string());
    return false;
}

bool ProjectSerializer::Deserialize(const std::filesystem::path &filepath)
{
    std::ifstream stream(filepath);
    if (!stream.is_open())
    {
        CH_CORE_ERROR("Failed to open project file: {0}", filepath.string());
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
    if (projectNode["ActiveScene"])
        config.ActiveScenePath = projectNode["ActiveScene"].as<std::string>();
    config.ProjectDirectory = filepath.parent_path();

    return true;
}
} // namespace CHEngine
