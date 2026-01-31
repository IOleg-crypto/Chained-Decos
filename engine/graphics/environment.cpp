#include "environment.h"
#include "engine/core/log.h"
#include "engine/core/yaml.h"
#include "yaml-cpp/yaml.h"
#include <filesystem>
#include <fstream>

namespace CHEngine
{

std::shared_ptr<EnvironmentAsset> EnvironmentAsset::Load(const std::string &path)
{
    auto asset = std::make_shared<EnvironmentAsset>();
    if (asset->Deserialize(path))
    {
        asset->SetPath(path);
        return asset;
    }
    return nullptr;
}

bool EnvironmentAsset::Deserialize(const std::string &path)
{
    std::filesystem::path fullPath(path);
    std::ifstream stream(fullPath);
    if (!stream.is_open())
    {
        CH_CORE_ERROR("Failed to open environment file: {0}", path);
        return false;
    }

    try
    {
        YAML::Node data = YAML::Load(stream);
        if (!data["Environment"])
            return false;

        auto envNode = data["Environment"];
        if (envNode["LightDirection"])
            m_Settings.LightDirection = envNode["LightDirection"].as<Vector3>();
        if (envNode["LightColor"])
            m_Settings.LightColor = envNode["LightColor"].as<Color>();
        if (envNode["AmbientIntensity"])
            m_Settings.AmbientIntensity = envNode["AmbientIntensity"].as<float>();

        auto skybox = envNode["Skybox"];
        if (skybox)
        {
            m_Settings.Skybox.TexturePath = skybox["TexturePath"].as<std::string>();
            m_Settings.Skybox.Exposure = skybox["Exposure"].as<float>();
            m_Settings.Skybox.Brightness = skybox["Brightness"].as<float>();
            m_Settings.Skybox.Contrast = skybox["Contrast"].as<float>();
        }

        return true;
    }
    catch (const std::exception &e)
    {
        CH_CORE_ERROR("Failed to parse environment file {0}: {1}", path, e.what());
        return false;
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

    std::filesystem::path fullPath(path);
    std::filesystem::create_directories(fullPath.parent_path());
    std::ofstream fout(fullPath);
    if (fout.is_open())
    {
        fout << out.c_str();
        return true;
    }
    return false;
}

void EnvironmentAsset::LoadFromFile(const std::string &path)
{
    if (m_State == AssetState::Ready) return;

    if (Deserialize(path))
    {
        SetState(AssetState::Ready);
    }
    else
    {
        SetState(AssetState::Failed);
    }
}

void EnvironmentAsset::UploadToGPU()
{
}
} // namespace CHEngine
