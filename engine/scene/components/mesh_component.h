#ifndef CH_MESH_COMPONENT_H
#define CH_MESH_COMPONENT_H

#include "engine/core/base.h"
#include "engine/graphics/asset.h"
#include "engine/graphics/material.h"
#include <string>
#include <vector>

namespace CHEngine
{
class ModelAsset;

enum class MaterialSlotTarget : uint8_t
{
    MaterialIndex = 0,
    MeshIndex = 1
};

struct MaterialSlot
{
    std::string Name;
    int Index = -1;
    MaterialSlotTarget Target = MaterialSlotTarget::MaterialIndex;
    MaterialInstance Material;

    MaterialSlot() = default;
    MaterialSlot(const std::string& name, int index)
        : Name(name),
          Index(index)
    {
    }
};

struct ModelComponent
{
    AssetHandle ModelHandle = 0;
    std::string ModelPath;
    std::shared_ptr<ModelAsset> Asset; // Cached asset reference
    std::vector<MaterialSlot> Materials;
    bool  MaterialsInitialized = false;
    float CullDistance = 0.0f; // 0 = no limit

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
