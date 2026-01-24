#include "environment.h"
#include "asset_manager.h"
#include "engine/core/log.h"
#include "engine/scene/yaml_utils.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{

// YAML conversion helpers (internal to this file or shared if needed)
// Using those from SceneSerializer if possible, but for a standalone asset
// we might need them here too if they are not in a common header.

Ref<EnvironmentAsset> EnvironmentAsset::Load(const std::string &path)
{
    auto fullPath = Assets::ResolvePath(path);
    std::ifstream stream(fullPath);
    if (!stream.is_open())
    {
        CH_CORE_ERROR("Failed to open environment file: {0}", path);
        return nullptr;
    }

    try
    {
        YAML::Node data = YAML::Load(stream);
        if (!data["Environment"])
            return nullptr;

        auto asset = std::make_shared<EnvironmentAsset>();
        auto &settings = asset->m_Settings;

        auto env = data["Environment"];
        if (env["LightDirection"])
            settings.LightDirection = env["LightDirection"].as<Vector3>();
        if (env["LightColor"])
            settings.LightColor = env["LightColor"].as<Color>();
        if (env["AmbientIntensity"])
            settings.AmbientIntensity = env["AmbientIntensity"].as<float>();

        auto skybox = env["Skybox"];
        if (skybox)
        {
            settings.Skybox.TexturePath = skybox["TexturePath"].as<std::string>();
            settings.Skybox.Exposure = skybox["Exposure"].as<float>();
            settings.Skybox.Brightness = skybox["Brightness"].as<float>();
            settings.Skybox.Contrast = skybox["Contrast"].as<float>();
        }

        asset->SetPath(path);
        return asset;
    }
    catch (const std::exception &e)
    {
        CH_CORE_ERROR("Failed to parse environment file {0}: {1}", path, e.what());
        return nullptr;
    }
}

bool EnvironmentAsset::Save(const std::string &path)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Environment" << YAML::BeginMap;

    out << YAML::Key << "LightDirection" << YAML::Value << m_Settings.LightDirection;
    out << YAML::Key << "LightColor" << YAML::Value << m_Settings.LightColor;
    out << YAML::Key << "AmbientIntensity" << YAML::Value << m_Settings.AmbientIntensity;

    out << YAML::Key << "Skybox" << YAML::BeginMap;
    out << YAML::Key << "TexturePath" << YAML::Value << m_Settings.Skybox.TexturePath;
    out << YAML::Key << "Exposure" << YAML::Value << m_Settings.Skybox.Exposure;
    out << YAML::Key << "Brightness" << YAML::Value << m_Settings.Skybox.Brightness;
    out << YAML::Key << "Contrast" << YAML::Value << m_Settings.Skybox.Contrast;
    out << YAML::EndMap;

    out << YAML::EndMap;
    out << YAML::EndMap;

    auto fullPath = Assets::ResolvePath(path);
    std::filesystem::create_directories(fullPath.parent_path());
    std::ofstream fout(fullPath);
    if (fout.is_open())
    {
        fout << out.c_str();
        return true;
    }
    return false;
}

} // namespace CHEngine
