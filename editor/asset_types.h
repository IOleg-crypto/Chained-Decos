#ifndef CH_EDITOR_ASSET_TYPES_H
#define CH_EDITOR_ASSET_TYPES_H

#include <cstdint>
#include <filesystem>
#include <string>

namespace Chained
{

enum class EditorAssetType
{
    Directory,
    Scene,
    Script,
    Model,
    Texture,
    Audio,
    Prefab,
    Shader,
    Other
};

struct AssetEntry
{
    std::string name;
    std::filesystem::path path;
    EditorAssetType type;
    uint32_t icon = 0;
    bool isDirectory = false;
};

} // namespace Chained

#endif // CH_EDITOR_ASSET_TYPES_H
