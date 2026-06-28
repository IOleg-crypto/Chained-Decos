#include "skybox_pass.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/texture_utility.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/assets/types/model_asset.h"

#include <algorithm>

namespace Chained {

    void SkyboxPass::Execute(const RenderContext& ctx)
    {
        auto environment = ctx.Settings.Environment;
        if (!environment)
        {
            environment = ctx.Options.EnvironmentOverride;
        }

        if (environment)
        {
            const auto& envSettings = environment->GetSettings();
            const auto& skySettings = envSettings.Skybox;
            if (!skySettings.TexturePath.empty())
            {
                auto handle = ServiceLocator::Get<AssetManager>()->ImportAsset(skySettings.TexturePath);
                auto textureAsset = ServiceLocator::Get<AssetManager>()->GetAsset<TextureAsset>(handle);
                if (textureAsset && textureAsset->IsReady() && textureAsset->GetTexture())
                {
                    int skyboxMode = std::clamp(skySettings.Mode, 0, 2);
                    uint32_t texId = textureAsset->GetTexture()->GetRendererID();

                    // Logic mapped directly from old SceneRenderer implementation
                    ServiceLocator::Get<Renderer>()->DrawSkybox(texId, skyboxMode, textureAsset->IsHDR(), skySettings.Exposure,
                                        skySettings.Brightness, skySettings.Contrast, ctx.Camera, skySettings.VFlipped);
                }
            }
        }
    }

}
