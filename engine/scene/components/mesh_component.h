#ifndef CH_MESH_COMPONENT_H
#define CH_MESH_COMPONENT_H

#include "engine/core/reflection_rfl.h"
#include "engine/assets/asset.h"
#include <string>
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


};

CH_MARK_RFL(MaterialSlot);

struct ModelComponent
{
    AssetHandle ModelHandle = AssetHandle(0);
    std::string ModelPath;
    std::vector<MaterialSlot> Materials;
    bool MaterialsInitialized = false;



    static const char* GetStaticName() { return "ModelComponent"; }
};

CH_MARK_RFL(ModelComponent);

struct MaterialComponent
{
    std::vector<MaterialSlot> Materials;
    bool MaterialsInitialized = false;



    static const char* GetStaticName() { return "MaterialComponent"; }
};

CH_MARK_RFL(MaterialComponent);

} // namespace CHEngine

#endif // CH_MESH_COMPONENT_H
