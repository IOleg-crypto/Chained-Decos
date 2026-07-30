#ifndef CH_ASSET_METADATA_H
#define CH_ASSET_METADATA_H

#include "engine/assets/asset.h"
#include "engine/common/uuid.h"
#include <filesystem>
#include <string>
#include <vector>

namespace YAML
{
class Node;
class Emitter;
} // namespace YAML

namespace Chained
{
struct AssetMetadata
{
    uint32_t version = 1;
    UUID uuid;
    AssetType assetType = AssetType::None;
    uint64_t contentHash = 0;
    std::string importerSettingsYaml;
    std::vector<std::string> tags;
    bool isGenerated = false;
};

namespace MetaUtils
{
std::filesystem::path GetMetaPath(const std::filesystem::path& assetPath);

bool HasMeta(const std::filesystem::path& assetPath);

uint64_t ComputeFileHash(const std::filesystem::path& path);

AssetMetadata ReadMeta(const std::filesystem::path& metaPath);

bool WriteMeta(const std::filesystem::path& metaPath, const AssetMetadata& meta);

AssetMetadata LoadOrCreateMeta(const std::filesystem::path& assetPath, AssetType type);

const char* AssetTypeToString(AssetType type);
AssetType StringToAssetType(const std::string& str);
} // namespace MetaUtils
} // namespace Chained

#endif
