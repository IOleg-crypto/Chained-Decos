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
        CH_PROP(props, Name);
        CH_PROP(props, Index);
        CH_PROP_NAMED(props, "Target", (int&)Target);
        CH_NESTED_NAMED(props, "Material", Material);
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
        CH_HEADER(props, "Model Asset");
        if (props.GetMode() != CHEngine::ReflectionMode::UI)
            CH_HANDLE_NAMED(props, "Handle", ModelHandle);
        
        CH_FILE(props, ModelPath, "fbx,gltf,glb,obj");

        if (props.GetMode() != CHEngine::ReflectionMode::UI)
        {
            CH_SEQUENCE_NAMED(props, "Materials", Materials);
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
        CH_HEADER(props, "Material Overrides");
        CH_SEQUENCE_NAMED(props, "Slots", Materials);
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_MESH_COMPONENT_H
