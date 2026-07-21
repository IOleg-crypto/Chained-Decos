#include "geometry_pass.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/pipeline/frustum.h"
#include "engine/graphics/api/graphics_device.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Chained {

    void GeometryPass::Execute(const RenderContext& ctx)
    {
        auto& renderer = *ctx.Renderer;

        // 1. Opaque Pass — no blending
        for (const auto& item : renderer.GetOpaqueQueue())
        {
            renderer.DrawModel(item.Asset, item.Transform, item.BoneMatrices, item.Materials, item.ShaderOverride,
                      item.CustomUniforms, RenderPassStage::Opaque);
        }

        // 2. Transparent Pass — enable blending
        GraphicsDevice::Get().SetBlendEnabled(true);
        GraphicsDevice::Get().SetBlendFunc(GraphicsDevice::BlendFactor::SrcAlpha, GraphicsDevice::BlendFactor::OneMinusSrcAlpha);
        GraphicsDevice::Get().DisableDepthMask();
        for (const auto& item : renderer.GetTransparentQueue())
        {
            renderer.DrawModel(item.Asset, item.Transform, item.BoneMatrices, item.Materials, item.ShaderOverride,
                      item.CustomUniforms, RenderPassStage::Transparent);
        }
        GraphicsDevice::Get().EnableDepthMask();
        GraphicsDevice::Get().SetBlendEnabled(false);
    }

}
