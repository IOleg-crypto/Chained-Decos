#include "renderer_types.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/core/assets/asset_manager.h"

namespace CHEngine
{

Model Model::CreateFromFile(const std::string& path)
{
    auto asset = AssetManager::Get().Get<ModelAsset>(path);
    if (asset && asset->GetState() == AssetState::Ready)
    {
        return asset->GetModel();
    }
    return Model{};
}

} // namespace CHEngine
