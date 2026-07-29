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

void MaterialAsset::SaveToFile(const std::string& path) const
{
    try
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "Name" << YAML::Value << m_Material.Name;
        out << YAML::Key << "AlbedoColor" << YAML::Value << Vec4ToYAML(m_Material.AlbedoColor);
        out << YAML::Key << "EmissiveColor" << YAML::Value << Vec4ToYAML(m_Material.EmissiveColor);
        out << YAML::Key << "EmissiveIntensity" << YAML::Value << m_Material.EmissiveIntensity;
        out << YAML::Key << "Metalness" << YAML::Value << m_Material.Metalness;
        out << YAML::Key << "Roughness" << YAML::Value << m_Material.Roughness;
        out << YAML::Key << "Transparent" << YAML::Value << m_Material.Transparent;
        out << YAML::Key << "Alpha" << YAML::Value << m_Material.Alpha;

        if (!m_Material.AlbedoPath.empty())
        {
            out << YAML::Key << "AlbedoMap" << YAML::Value << m_Material.AlbedoPath;
        }
        if (!m_Material.NormalPath.empty())
        {
            out << YAML::Key << "NormalMap" << YAML::Value << m_Material.NormalPath;
        }
        if (!m_Material.MetallicRoughnessPath.empty())
        {
            out << YAML::Key << "MetallicRoughnessMap" << YAML::Value << m_Material.MetallicRoughnessPath;
        }
        if (!m_Material.EmissivePath.empty())
        {
            out << YAML::Key << "EmissiveMap" << YAML::Value << m_Material.EmissivePath;
        }
        if (!m_Material.OcclusionPath.empty())
        {
            out << YAML::Key << "OcclusionMap" << YAML::Value << m_Material.OcclusionPath;
        }

        out << YAML::EndMap;
        out << YAML::EndMap;

        std::ofstream file(path);
        if (!file.is_open())
        {
            CH_CORE_ERROR("MaterialAsset: Failed to open file for writing: {}", path);
            return;
        }
        file << out.c_str();
        CH_CORE_INFO("MaterialAsset: Saved to {}", path);
    } catch (const YAML::Exception& e)
    {
        CH_CORE_ERROR("MaterialAsset: Failed to save {}: {}", path, e.what());
    }
}

bool MaterialAsset::LoadFromFile(const std::string& path, std::string* outError)
{
    std::string content;

    // Try reading from pack first
    if (auto* am = ServiceLocator::TryGet<AssetManager>())
    {
        if (am->IsPacked())
        {
            auto data = am->ReadAssetData(path);
            if (!data.empty())
            {
                content.assign(data.begin(), data.end());
            }
        }
    }

    // Fallback to disk
    if (content.empty())
    {
        if (!std::filesystem::exists(path))
        {
            if (outError)
            {
                *outError = "MaterialAsset: File not found: " + path;
            }
            return false;
        }
        std::ifstream stream(path);
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
                *outError = "MaterialAsset: Missing 'Material' key in " + path;
            }
            return false;
        }

        YAML::Node mat = root["Material"];

        if (mat["Name"])
        {
            m_Material.Name = mat["Name"].as<std::string>();
        }
        if (mat["AlbedoColor"])
        {
            m_Material.AlbedoColor = Vec4FromYAML(mat["AlbedoColor"]);
        }
        if (mat["EmissiveColor"])
        {
            m_Material.EmissiveColor = Vec4FromYAML(mat["EmissiveColor"]);
        }
        if (mat["EmissiveIntensity"])
        {
            m_Material.EmissiveIntensity = mat["EmissiveIntensity"].as<float>();
        }
        if (mat["Metalness"])
        {
            m_Material.Metalness = mat["Metalness"].as<float>();
        }
        if (mat["Roughness"])
        {
            m_Material.Roughness = mat["Roughness"].as<float>();
        }
        if (mat["Transparent"])
        {
            m_Material.Transparent = mat["Transparent"].as<bool>();
        }
        if (mat["Alpha"])
        {
            m_Material.Alpha = mat["Alpha"].as<float>();
        }
        if (mat["AlbedoMap"])
        {
            m_Material.AlbedoPath = mat["AlbedoMap"].as<std::string>();
        }
        if (mat["NormalMap"])
        {
            m_Material.NormalPath = mat["NormalMap"].as<std::string>();
        }
        if (mat["MetallicRoughnessMap"])
        {
            m_Material.MetallicRoughnessPath = mat["MetallicRoughnessMap"].as<std::string>();
        }
        if (mat["EmissiveMap"])
        {
            m_Material.EmissivePath = mat["EmissiveMap"].as<std::string>();
        }
        if (mat["OcclusionMap"])
        {
            m_Material.OcclusionPath = mat["OcclusionMap"].as<std::string>();
        }
    } catch (const YAML::Exception& e)
    {
        if (outError)
        {
            *outError = std::string("MaterialAsset: YAML parse error in ") + path + ": " + e.what();
        }
        return false;
    }

    return true;
}

} // namespace Chained
