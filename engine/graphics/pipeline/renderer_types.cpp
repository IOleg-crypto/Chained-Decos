#include "renderer_types.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"

namespace CHEngine
{

Model Model::CreateFromFile(const std::string& path)
{
    auto handle = ServiceLocator::Get<AssetManager>().ResolveToHandle(path, ModelAsset::GetStaticType());
    auto asset = ServiceLocator::Get<AssetManager>().Get<ModelAsset>(handle);
    if (asset && asset->GetState() == AssetState::Ready)
    {
        return asset->GetModel();
    }
    return Model{};
}

} // namespace CHEngine
