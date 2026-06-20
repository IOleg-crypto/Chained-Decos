#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/assets/loaders/asset_importer.h"
#include "engine/core/log.h"
#include "engine/serialization/yaml_conversions.h"
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Chained::AssetImporter
{
std::shared_ptr<EnvironmentAsset> ImportEnvironment(AssetHandle handle, const AssetMetadata& metadata)
{
    std::filesystem::path fullPath = ServiceLocator::Get<AssetManager>()->GetAssetDirectory() / metadata.FilePath;
    std::ifstream stream(fullPath);

    if (!stream.is_open())
    {
        return nullptr;
    }

    try
    {
        YAML::Node data = YAML::Load(stream);
        if (!data["Environment"])
        {
            return nullptr;
        }

        auto envNode = data["Environment"];
        EnvironmentParameters settings;

        if (envNode["Lighting"])
        {
            auto lighting = envNode["Lighting"];
            if (lighting["Direction"])
            {
                settings.Lighting.Direction = lighting["Direction"].as<glm::vec3>();
            }
            if (lighting["LightColor"])
            {
                settings.Lighting.LightColor = lighting["LightColor"].as<Chained::Color>();
            }
            if (lighting["Ambient"])
            {
                settings.Lighting.Ambient = lighting["Ambient"].as<float>();
            }
            if (lighting["Exposure"])
            {
                settings.Lighting.Exposure = lighting["Exposure"].as<float>();
            }
            if (lighting["Gamma"])
            {
                settings.Lighting.Gamma = lighting["Gamma"].as<float>();
            }
        }
        else
        {
            if (envNode["LightDirection"])
            {
                settings.Lighting.Direction = envNode["LightDirection"].as<glm::vec3>();
            }
            if (envNode["LightColor"])
            {
                settings.Lighting.LightColor = envNode["LightColor"].as<Chained::Color>();
            }
            if (envNode["AmbientIntensity"])
            {
                settings.Lighting.Ambient = envNode["AmbientIntensity"].as<float>();
            }
        }

        auto skyboxNode = envNode["Skybox"];
        if (skyboxNode)
        {
            if (skyboxNode["TexturePath"])
            {
                settings.Skybox.TexturePath = skyboxNode["TexturePath"].as<std::string>();
            }
            if (skyboxNode["Mode"])
            {
                settings.Skybox.Mode = skyboxNode["Mode"].as<int>();
            }
            if (skyboxNode["Exposure"])
            {
                settings.Skybox.Exposure = skyboxNode["Exposure"].as<float>();
            }
            if (skyboxNode["Brightness"])
            {
                settings.Skybox.Brightness = skyboxNode["Brightness"].as<float>();
            }
            if (skyboxNode["Contrast"])
            {
                settings.Skybox.Contrast = skyboxNode["Contrast"].as<float>();
            }
            if (skyboxNode["VFlipped"])
            {
                settings.Skybox.VFlipped = skyboxNode["VFlipped"].as<bool>();
            }
        }

        auto fogNode = envNode["Fog"];
        if (fogNode)
        {
            if (fogNode["Enabled"])
            {
                settings.Fog.Enabled = fogNode["Enabled"].as<bool>();
            }
            if (fogNode["Color"])
            {
                settings.Fog.FogColor = fogNode["Color"].as<Chained::Color>();
            }
            if (fogNode["Density"])
            {
                settings.Fog.Density = fogNode["Density"].as<float>();
            }
            if (fogNode["Start"])
            {
                settings.Fog.Start = fogNode["Start"].as<float>();
            }
            if (fogNode["End"])
            {
                settings.Fog.End = fogNode["End"].as<float>();
            }
        }

        auto asset = std::make_shared<EnvironmentAsset>(handle);
        asset->SetPath(metadata.FilePath.string());
        asset->SetSettings(settings);
        asset->SetState(AssetState::Ready);

        return asset;
    } catch (const std::exception& e)
    {
        CH_CORE_ERROR("EnvironmentAsset: Failed to parse environment file {0}: {1}", fullPath.string(), e.what());
        return nullptr;
    }
}
} // namespace Chained::AssetImporter
