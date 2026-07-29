#include "engine/assets/loaders/material_loader.h"
#include "engine/assets/types/material_asset.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/log.h"
#include "engine/core/service_locator.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <filesystem>
#include <sstream>

namespace Chained
{

std::shared_ptr<Asset> MaterialLoader::Create()
{
    return std::make_shared<MaterialAsset>();
}

static YAML::Node Vec4ToYAML(const glm::vec4& v)
{
    YAML::Node node;
    node.push_back(v.x);
    node.push_back(v.y);
    node.push_back(v.z);
    node.push_back(v.w);
    return node;
}

static glm::vec4 Vec4FromYAML(const YAML::Node& node)
{
    if (!node || !node.IsSequence() || node.size() < 4)
    {
        return {0, 0, 0, 1};
    }
    return {node[0].as<float>(), node[1].as<float>(), node[2].as<float>(), node[3].as<float>()};
}

bool MaterialLoader::Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError)
{
    auto matAsset = std::static_pointer_cast<MaterialAsset>(asset);

    std::string content;

    // Try reading from pack first
    if (auto* am = ServiceLocator::TryGet<AssetManager>())
    {
        if (am->IsPacked())
        {
            auto data = am->ReadAssetData(resolvedPath);
            if (!data.empty())
            {
                content.assign(data.begin(), data.end());
            }
        }
    }

    // Fallback to disk
    if (content.empty())
    {
        if (!std::filesystem::exists(resolvedPath))
        {
            if (outError)
            {
                *outError = "MaterialLoader: File not found: " + resolvedPath;
            }
            return false;
        }
        std::ifstream stream(resolvedPath);
        std::stringstream ss;
        ss << stream.rdbuf();
        content = ss.str();
    }

    try
    {
        YAML::Node root = YAML::Load(content);
        if (!root["Material"])
        {
            if (outError)
            {
                *outError = "MaterialLoader: Missing 'Material' key in " + resolvedPath;
            }
            return false;
        }

        YAML::Node mat = root["Material"];
        Material& m = matAsset->GetMaterial();

        if (mat["Name"])
        {
            m.Name = mat["Name"].as<std::string>();
        }
        if (mat["AlbedoColor"])
        {
            m.AlbedoColor = Vec4FromYAML(mat["AlbedoColor"]);
        }
        if (mat["EmissiveColor"])
        {
            m.EmissiveColor = Vec4FromYAML(mat["EmissiveColor"]);
        }
        if (mat["EmissiveIntensity"])
        {
            m.EmissiveIntensity = mat["EmissiveIntensity"].as<float>();
        }
        if (mat["Metalness"])
        {
            m.Metalness = mat["Metalness"].as<float>();
        }
        if (mat["Roughness"])
        {
            m.Roughness = mat["Roughness"].as<float>();
        }
        if (mat["Transparent"])
        {
            m.Transparent = mat["Transparent"].as<bool>();
        }
        if (mat["Alpha"])
        {
            m.Alpha = mat["Alpha"].as<float>();
        }
        if (mat["AlbedoMap"])
        {
            m.AlbedoPath = mat["AlbedoMap"].as<std::string>();
        }
        if (mat["NormalMap"])
        {
            m.NormalPath = mat["NormalMap"].as<std::string>();
        }
        if (mat["MetallicRoughnessMap"])
        {
            m.MetallicRoughnessPath = mat["MetallicRoughnessMap"].as<std::string>();
        }
        if (mat["EmissiveMap"])
        {
            m.EmissivePath = mat["EmissiveMap"].as<std::string>();
        }
        if (mat["OcclusionMap"])
        {
            m.OcclusionPath = mat["OcclusionMap"].as<std::string>();
        }
    } catch (const YAML::Exception& e)
    {
        if (outError)
        {
            *outError = std::string("MaterialLoader: YAML parse error in ") + resolvedPath + ": " + e.what();
        }
        return false;
    }

    return true;
}

} // namespace Chained
