#ifndef CH_MESH_COMPONENT_H
#define CH_MESH_COMPONENT_H

#include "engine/core/base.h"
#include "engine/core/assets/asset.h"
#include "engine/graphics/pipeline/material.h"
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

    template <typename Archive>
    static void Serialize(Archive& archive, MaterialSlot& slot)
    {
        archive.Property("Name", slot.Name)
            .Property("Index", slot.Index)
            .Property("Target", (int&)slot.Target);
        
        // MaterialInstance serialization is handled by archive normally
        archive.Property("Material", slot.Material);
    }
};

struct ModelComponent
{
    AssetHandle ModelHandle = 0;
    std::string ModelPath;
    std::vector<MaterialSlot> Materials;
    bool  MaterialsInitialized = false;

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

    static const char* GetStaticName() { return "ModelComponent"; }

    template <typename Archive>
    static void Serialize(Archive& archive, ModelComponent& component)
    {
        archive.Handle("ModelHandle", component.ModelHandle)
            .Path("ModelPath", component.ModelPath)
            .Property("Materials", component.Materials);
        
        if (archive.GetMode() == SerializationUtils::PropertyArchive::Deserialize)
            component.MaterialsInitialized = true;
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
