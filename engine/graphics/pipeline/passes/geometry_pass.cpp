#include "geometry_pass.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/pipeline/frustum.h"
#include "engine/graphics/pipeline/render_command.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace Chained {

    void GeometryPass::Execute(const RenderContext& ctx)
    {
        auto& renderer = *ctx.Renderer;

        RenderCommand::SetBlendMode(true);
        RenderCommand::SetBlendFunc(RendererAPI::BlendFactor::SrcAlpha, RendererAPI::BlendFactor::OneMinusSrcAlpha);

        // 1. Opaque Pass
        for (const auto& item : renderer.GetOpaqueQueue())
        {
            renderer.DrawModel(item.Asset, item.Transform, item.BoneMatrices, item.Materials, item.ShaderOverride,
                      item.CustomUniforms, RenderPassStage::Opaque);
        }

        // 2. Transparent Pass
        RenderCommand::DisableDepthMask();
        for (const auto& item : renderer.GetTransparentQueue())
        {
            renderer.DrawModel(item.Asset, item.Transform, item.BoneMatrices, item.Materials, item.ShaderOverride,
                      item.CustomUniforms, RenderPassStage::Transparent);
        }
        RenderCommand::EnableDepthMask();
        RenderCommand::SetBlendMode(false);
    }

}
