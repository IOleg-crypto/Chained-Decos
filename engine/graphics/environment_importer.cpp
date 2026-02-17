#include "environment_importer.h"
#include "engine/core/log.h"
#include "engine/core/yaml.h"
#include "yaml-cpp/yaml.h"
#include <filesystem>
#include <fstream>

namespace CHEngine
{
    std::shared_ptr<EnvironmentAsset> EnvironmentImporter::ImportEnvironment(const std::string& path)
    {
        std::filesystem::path fullPath(path);
        std::ifstream stream(fullPath);
        if (!stream.is_open())
        {
            CH_CORE_ERROR("EnvironmentImporter: Failed to open environment file: {0}", path);
            return nullptr;
        }

        auto asset = std::make_shared<EnvironmentAsset>();
        asset->SetPath(path);

        try
        {
            YAML::Node data = YAML::Load(stream);
            if (!data["Environment"])
                return nullptr;

            auto envNode = data["Environment"];
            auto& settings = asset->GetSettings();

            // New format: Lighting section
            if (envNode["Lighting"])
            {
                auto lighting = envNode["Lighting"];
                if (lighting["Direction"]) settings.Lighting.Direction = lighting["Direction"].as<Vector3>();
                if (lighting["LightColor"]) settings.Lighting.LightColor = lighting["LightColor"].as<Color>();
                if (lighting["Ambient"]) settings.Lighting.Ambient = lighting["Ambient"].as<float>();
            }
            else
            {
                // Backward compat: old flat field names
                if (envNode["LightDirection"]) settings.Lighting.Direction = envNode["LightDirection"].as<Vector3>();
                if (envNode["LightColor"]) settings.Lighting.LightColor = envNode["LightColor"].as<Color>();
                if (envNode["AmbientIntensity"]) settings.Lighting.Ambient = envNode["AmbientIntensity"].as<float>();
            }

            auto skybox = envNode["Skybox"];
            if (skybox)
            {
                settings.Skybox.TexturePath = skybox["TexturePath"].as<std::string>();
                settings.Skybox.Exposure = skybox["Exposure"].as<float>();
                settings.Skybox.Brightness = skybox["Brightness"].as<float>();
                settings.Skybox.Contrast = skybox["Contrast"].as<float>();
            }

            auto fog = envNode["Fog"];
            if (fog)
            {
                settings.Fog.Enabled = fog["Enabled"].as<bool>();
                settings.Fog.FogColor = fog["Color"].as<Color>();
                settings.Fog.Density = fog["Density"].as<float>();
                settings.Fog.Start = fog["Start"].as<float>();
                settings.Fog.End = fog["End"].as<float>();
            }

            asset->SetState(AssetState::Ready);
            return asset;
        }
        catch (const std::exception &e)
        {
            CH_CORE_ERROR("EnvironmentImporter: Failed to parse environment file {0}: {1}", path, e.what());
            asset->SetState(AssetState::Failed);
            return nullptr;
        }
    }

    bool EnvironmentImporter::SaveEnvironment(const std::shared_ptr<EnvironmentAsset>& asset, const std::string& path)
    {
        if (!asset) return false;

        const auto& settings = asset->GetSettings();

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Environment" << YAML::BeginMap;

        out << YAML::Key << "Lighting" << YAML::BeginMap;
        out << YAML::Key << "Direction" << YAML::Value << settings.Lighting.Direction;
        out << YAML::Key << "LightColor" << YAML::Value << settings.Lighting.LightColor;
        out << YAML::Key << "Ambient" << YAML::Value << settings.Lighting.Ambient;
        out << YAML::EndMap;

        out << YAML::Key << "Skybox" << YAML::BeginMap;
        out << YAML::Key << "TexturePath" << YAML::Value << settings.Skybox.TexturePath;
        out << YAML::Key << "Exposure" << YAML::Value << settings.Skybox.Exposure;
        out << YAML::Key << "Brightness" << YAML::Value << settings.Skybox.Brightness;
        out << YAML::Key << "Contrast" << YAML::Value << settings.Skybox.Contrast;
        out << YAML::EndMap;

        out << YAML::Key << "Fog" << YAML::BeginMap;
        out << YAML::Key << "Enabled" << YAML::Value << settings.Fog.Enabled;
        out << YAML::Key << "Color" << YAML::Value << settings.Fog.FogColor;
        out << YAML::Key << "Density" << YAML::Value << settings.Fog.Density;
        out << YAML::Key << "Start" << YAML::Value << settings.Fog.Start;
        out << YAML::Key << "End" << YAML::Value << settings.Fog.End;
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
}
