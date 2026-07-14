#ifndef CH_MODEL_COMPONENT_H
#define CH_MODEL_COMPONENT_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/renderer_types.h"
#include "engine/reflection/reflection.h"
#include "engine/reflection/reflection_rfl.h"
#include <string>
#include <vector>

namespace Chained
{
struct ModelComponent
{
    AssetHandle ModelHandle = AssetHandle(0);
    std::string ModelPath;
    std::vector<Material> Materials;

    static const char* GetStaticName()
    {
        return "ModelComponent";
    }

    
    struct UI
    {
        UIMeta ModelPath = {.Hint = PropertyMeta::WidgetHint::FilePicker, .Extensions = ".glb,.gltf,.obj"};
    };
};

CH_MARK_RFL(ModelComponent);

} // namespace Chained

#endif // CH_MODEL_COMPONENT_H