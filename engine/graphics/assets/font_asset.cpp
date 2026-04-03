#include "engine/graphics/assets/font_asset.h"
#include "engine/core/assets/asset_manager.h"

namespace CHEngine
{
    NativeFont FontAsset::CreateFromFile(const std::string& path)
    {
        auto asset = AssetManager::Get().Get<FontAsset>(path);
        if (asset && asset->GetState() == AssetState::Ready)
        {
            return asset->GetFont();
        }
        return NativeFont{};
    }
}
