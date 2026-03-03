#ifndef CH_MESH_COMPONENT_H
#define CH_MESH_COMPONENT_H

#include "engine/core/base.h"
#include "engine/graphics/asset.h"
#include "engine/graphics/graphics_types.h"
#include <string>
#include <vector>

namespace CHEngine
{
class ModelAsset;

struct ModelComponent
{
    AssetHandle ModelHandle = 0;
    std::string ModelPath;
    std::shared_ptr<ModelAsset> Asset; // Cached asset reference
    std::vector<MaterialSlot> Materials;
    bool MaterialsInitialized = false;

    ModelComponent() = default;
    ModelComponent(const ModelComponent&) = default;
    ModelComponent(AssetHandle handle)
        : ModelHandle(handle)
    {
    }
    ModelComponent(const std::string& path)
        : ModelPath(path)
    {
    }
};

struct MaterialComponent
{
    std::vector<MaterialSlot> Slots;

    MaterialComponent() = default;
    MaterialComponent(const MaterialComponent&) = default;
};

} // namespace CHEngine

#endif // CH_MESH_COMPONENT_H
