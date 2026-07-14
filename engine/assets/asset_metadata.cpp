#include "engine/assets/asset_metadata.h"
#include "engine/core/log.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <numeric>

namespace Chained
{

std::filesystem::path MetaUtils::GetMetaPath(const std::filesystem::path& assetPath)
{
    return assetPath.string() + ".meta";
}

bool MetaUtils::HasMeta(const std::filesystem::path& assetPath)
{
    return std::filesystem::exists(GetMetaPath(assetPath));
}

uint64_t MetaUtils::ComputeFileHash(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 0;
    auto size = file.tellg();
    if (size <= 0) return 0;
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(static_cast<size_t>(size));
    if (!file.read(buffer.data(), size)) return 0;
    return std::accumulate(buffer.begin(), buffer.end(), UINT64_C(14695981039346656037),
        [](uint64_t hash, char c) { return (hash ^ static_cast<uint64_t>(static_cast<unsigned char>(c))) * 1099511628211ULL; });
}

const char* MetaUtils::AssetTypeToString(AssetType type)
{
    switch (type)
    {
    case AssetType::Texture:     return "Texture";
    case AssetType::Model:       return "Model";
    case AssetType::Audio:       return "Audio";
    case AssetType::Shader:      return "Shader";
    case AssetType::Material:    return "Material";
    case AssetType::Environment: return "Environment";
    case AssetType::Scene:       return "Scene";
    case AssetType::Script:      return "Script";
    case AssetType::Font:        return "Font";
    default:                     return "Unknown";
    }
}

AssetType MetaUtils::StringToAssetType(const std::string& str)
{
    if (str == "Texture")     return AssetType::Texture;
    if (str == "Model")       return AssetType::Model;
    if (str == "Audio")       return AssetType::Audio;
    if (str == "Shader")      return AssetType::Shader;
    if (str == "Material")    return AssetType::Material;
    if (str == "Environment") return AssetType::Environment;
    if (str == "Scene")       return AssetType::Scene;
    if (str == "Script")      return AssetType::Script;
    if (str == "Font")        return AssetType::Font;
    return AssetType::None;
}

AssetMetadata MetaUtils::ReadMeta(const std::filesystem::path& metaPath)
{
    AssetMetadata meta;
    try
    {
        YAML::Node root = YAML::LoadFile(metaPath.string());

        meta.version = root["Version"] ? root["Version"].as<uint32_t>() : 1;
        meta.contentHash = root["ContentHash"] ? root["ContentHash"].as<uint64_t>() : 0;
        meta.isGenerated = root["IsGenerated"] ? root["IsGenerated"].as<bool>() : false;

        if (root["UUID"])
        {
            meta.uuid = UUID(root["UUID"].as<uint64_t>());
        }

        if (root["Type"])
        {
            meta.type = StringToAssetType(root["Type"].as<std::string>());
        }

        if (root["ImporterSettings"])
        {
            meta.importerSettingsYaml = YAML::Dump(root["ImporterSettings"]);
        }

        if (root["Tags"] && root["Tags"].IsSequence())
        {
            for (auto tag : root["Tags"])
            {
                meta.tags.push_back(tag.as<std::string>());
            }
        }
    }
    catch (const YAML::Exception& e)
    {
        CH_CORE_ERROR("Failed to parse meta file {}: {}", metaPath.string(), e.what());
        return {};
    }
    return meta;
}

bool MetaUtils::WriteMeta(const std::filesystem::path& metaPath, const AssetMetadata& meta)
{
    try
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Version" << YAML::Value << meta.version;
        out << YAML::Key << "UUID" << YAML::Value << static_cast<uint64_t>(meta.uuid);
        out << YAML::Key << "Type" << YAML::Value << AssetTypeToString(meta.type);
        out << YAML::Key << "ContentHash" << YAML::Value << meta.contentHash;
        out << YAML::Key << "IsGenerated" << YAML::Value << meta.isGenerated;

        if (!meta.tags.empty())
        {
            out << YAML::Key << "Tags" << YAML::Value << YAML::BeginSeq;
            for (const auto& tag : meta.tags)
            {
                out << tag;
            }
            out << YAML::EndSeq;
        }

        if (!meta.importerSettingsYaml.empty())
        {
            out << YAML::Key << "ImporterSettings" << YAML::Value << YAML::Load(meta.importerSettingsYaml);
        }

        out << YAML::EndMap;

        std::ofstream file(metaPath);
        if (!file.is_open())
        {
            CH_CORE_ERROR("Failed to open meta file for writing: {}", metaPath.string());
            return false;
        }
        file << out.c_str();
        return true;
    }
    catch (const YAML::Exception& e)
    {
        CH_CORE_ERROR("Failed to write meta file {}: {}", metaPath.string(), e.what());
        return false;
    }
}

AssetMetadata MetaUtils::LoadOrCreateMeta(const std::filesystem::path& assetPath, AssetType type)
{
    std::filesystem::path metaPath = GetMetaPath(assetPath);

    if (std::filesystem::exists(metaPath))
    {
        AssetMetadata meta = ReadMeta(metaPath);

        uint64_t currentHash = ComputeFileHash(assetPath);
        if (currentHash != 0 && meta.contentHash != currentHash)
        {
            meta.contentHash = currentHash;
            WriteMeta(metaPath, meta);
        }
        return meta;
    }

    AssetMetadata meta;
    meta.uuid = UUID();
    meta.type = type;
    meta.contentHash = ComputeFileHash(assetPath);
    meta.isGenerated = true;

    WriteMeta(metaPath, meta);
    return meta;
}

} // namespace Chained
