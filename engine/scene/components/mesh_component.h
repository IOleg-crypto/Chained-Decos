#ifndef CH_MESH_COMPONENT_H
#define CH_MESH_COMPONENT_H

#include "engine/core/reflection.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/graphics/pipeline/material.h"

namespace CHEngine
{
class ModelAsset;
class AssetManager;

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

    CH_REFLECT_BEGIN(MaterialSlot)
        props.Property("Name", Name);
        props.Property("Index", Index);
        props.Property("Target", (int&)Target);
        props.Nested("Material", Material);
    CH_REFLECT_END()
};

struct ModelComponent
{
    AssetHandle ModelHandle = AssetHandle(0);
    std::string ModelPath;
    std::vector<MaterialSlot> Materials;
    bool MaterialsInitialized = false;

    ModelComponent() = default;
    ModelComponent(const ModelComponent&) = default;
    ModelComponent(AssetHandle handle)
        : ModelHandle(handle)
    {
    }
    CH_REFLECT_BEGIN(ModelComponent)
        props.Header("Model Asset");
        if (props.GetMode() != CHEngine::ReflectionMode::UI)
            props.Handle("Handle", ModelHandle);
        
        if (props.File("ModelPath", ModelPath, "fbx,gltf,glb,obj"))
        {
            // Logic moved to AssetResolutionSystem
        }

        if (props.GetMode() != CHEngine::ReflectionMode::UI)
        {
            props.Sequence("Materials", Materials);
        }
    CH_REFLECT_END()
};

struct MaterialComponent
{
    std::vector<MaterialSlot> Materials;
    bool MaterialsInitialized = false;

    MaterialComponent() = default;
    MaterialComponent(const MaterialComponent&) = default;

    CH_REFLECT_BEGIN(MaterialComponent)
        props.Header("Material Overrides");
        props.Sequence("Slots", Materials);
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_MESH_COMPONENT_H
