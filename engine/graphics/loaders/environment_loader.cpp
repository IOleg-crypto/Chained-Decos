#include "engine/graphics/loaders/environment_loader.h"
#include "engine/core/log.h"
#include "engine/scene/yaml.h"
#include "yaml-cpp/yaml.h"
#include <filesystem>
#include <fstream>

namespace CHEngine
{
    std::shared_ptr<Asset> EnvironmentLoader::Create()
    {
        return std::make_shared<EnvironmentAsset>();
    }

    bool EnvironmentLoader::Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath)
    {
        std::filesystem::path fullPath(resolvedPath);
        std::ifstream stream(fullPath);
        if (!stream.is_open())
        {
            CH_CORE_ERROR("EnvironmentLoader: Failed to open environment file: {0}", resolvedPath);
            return false;
        }

        auto envAsset = std::static_pointer_cast<EnvironmentAsset>(asset);

        try
        {
            YAML::Node data = YAML::Load(stream);
            if (!data["Environment"])
            {
                return false;
            }

            auto envNode = data["Environment"];
            EnvironmentSettings settings;

            // New format: Lighting section
            if (envNode["Lighting"])
            {
                auto lighting = envNode["Lighting"];
                if (lighting["Direction"])
                {
                    settings.Lighting.Direction = lighting["Direction"].as<glm::vec3>();
                }
                if (lighting["LightColor"])
                {
                    settings.Lighting.LightColor = lighting["LightColor"].as<CHEngine::Color>();
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
                // Backward compat: old flat field names
                if (envNode["LightDirection"])
                {
                    settings.Lighting.Direction = envNode["LightDirection"].as<glm::vec3>();
                }
                if (envNode["LightColor"])
                {
                    settings.Lighting.LightColor = envNode["LightColor"].as<CHEngine::Color>();
                }
                if (envNode["AmbientIntensity"])
                {
                    settings.Lighting.Ambient = envNode["AmbientIntensity"].as<float>();
                }
            }

            auto skyboxNode = envNode["Skybox"];
            if (skyboxNode)
            {
                settings.Skybox.TexturePath = skyboxNode["TexturePath"].as<std::string>();
                if (skyboxNode["Mode"]) settings.Skybox.Mode = skyboxNode["Mode"].as<int>();
                if (skyboxNode["Exposure"]) settings.Skybox.Exposure = skyboxNode["Exposure"].as<float>();
                if (skyboxNode["Brightness"]) settings.Skybox.Brightness = skyboxNode["Brightness"].as<float>();
                if (skyboxNode["Contrast"]) settings.Skybox.Contrast = skyboxNode["Contrast"].as<float>();
            }

            auto fogNode = envNode["Fog"];
            if (fogNode)
            {
                settings.Fog.Enabled = fogNode["Enabled"].as<bool>();
                settings.Fog.FogColor = fogNode["Color"].as<CHEngine::Color>();
                settings.Fog.Density = fogNode["Density"].as<float>();
                settings.Fog.Start = fogNode["Start"].as<float>();
                settings.Fog.End = fogNode["End"].as<float>();
            }

            envAsset->SetSettings(settings);
            return true;
        } catch (const std::exception& e)
        {
            CH_CORE_ERROR("EnvironmentLoader: Failed to parse environment file {0}: {1}", resolvedPath, e.what());
            return false;
        }
    }
} // namespace CHEngine
