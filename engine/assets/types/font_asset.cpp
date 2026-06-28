#include "engine/assets/types/font_asset.h"
#include "engine/core/service_locator.h"
#include "engine/assets/asset_manager.h"

namespace Chained
{
    NativeFont FontAsset::CreateFromFile(const std::string& path)
    {
        auto asset = ServiceLocator::Get<AssetManager>()->Get<FontAsset>(path);
        if (asset && asset->GetState() == AssetState::Ready)
        {
            return asset->GetFont();
        }
        return NativeFont{};
    }
}