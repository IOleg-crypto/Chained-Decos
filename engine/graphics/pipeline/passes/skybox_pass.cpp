#include "skybox_pass.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/assets/types/model_asset.h"
#include "types/environment_asset.h"

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
                auto* am = ServiceLocator::TryGet<AssetManager>();
                if (!am) return;
                auto handle = am->LoadAsset(skySettings.TexturePath, TextureAsset::GetStaticType());
                auto textureAsset = am->Get<TextureAsset>(skySettings.TexturePath);
                if (textureAsset && textureAsset->IsReady() && textureAsset->GetTexture())
                {
                    int skyboxMode = std::clamp(skySettings.Mode, 0, 2);
                    uint32_t texId = textureAsset->GetTexture()->GetNativeHandle();

                    // Logic mapped directly from old SceneRenderer implementation
                    if (auto* r = ServiceLocator::TryGet<Renderer>())
                        r->DrawSkybox(texId, skyboxMode, textureAsset->IsHDR(), skySettings.Exposure,
                                      skySettings.Brightness, skySettings.Contrast, ctx.Camera, true);
                }
            }
        }
    }

}
