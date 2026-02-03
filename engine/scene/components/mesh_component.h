#ifndef CH_MESH_COMPONENT_H
#define CH_MESH_COMPONENT_H

#include "engine/core/base.h"
#include "engine/graphics/material.h"
#include <string>
#include <vector>
#include "../reflect.h"

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
    MaterialSlot(const std::string &name, int index) : Name(name), Index(index)
    {
    }
};

struct ModelComponent
{
    std::string ModelPath;
    std::shared_ptr<ModelAsset> Asset; // Cached asset reference
    std::vector<MaterialSlot> Materials;
    bool MaterialsInitialized = false;

    ModelComponent() = default;
    ModelComponent(const ModelComponent &) = default;
    ModelComponent(const std::string &path) : ModelPath(path)
    {
    }
};

struct MaterialComponent
{
    std::vector<MaterialSlot> Slots;

    MaterialComponent() = default;
    MaterialComponent(const MaterialComponent &) = default;
};

BEGIN_REFLECT(ModelComponent)
    PROPERTY(std::string, ModelPath, "Model Path")
END_REFLECT()

} // namespace CHEngine

#endif // CH_MESH_COMPONENT_H
