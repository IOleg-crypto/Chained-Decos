#include "engine/graphics/asset_manager.h"
#include "engine/graphics/texture_asset.h"
#include "engine/graphics/model_asset.h"
#include "engine/audio/sound_asset.h"
#include "engine/graphics/shader_asset.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/font_asset.h"

namespace CHEngine
{
    // Need to specialize or implement the template calls here where headers are available
    
    void AssetManager::Update()
    {
        UpdateCache<TextureAsset>();
        UpdateCache<ModelAsset>();
        UpdateCache<SoundAsset>();
        UpdateCache<ShaderAsset>();
        UpdateCache<EnvironmentAsset>();
        UpdateCache<FontAsset>();
    }

} // namespace CHEngine
