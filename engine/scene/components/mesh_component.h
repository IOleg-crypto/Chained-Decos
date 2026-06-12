#ifndef CH_MESH_COMPONENT_H
#define CH_MESH_COMPONENT_H

#include "engine/reflection/reflection_rfl.h"
#include "engine/assets/asset.h"
#include <string>


namespace Chained
{
class ModelAsset;

struct ModelComponent
{
    AssetHandle ModelHandle = AssetHandle(0);
    std::string ModelPath;

    static const char* GetStaticName() { return "ModelComponent"; }
};

CH_MARK_RFL(ModelComponent);

} // namespace Chained

#endif // CH_MESH_COMPONENT_H
