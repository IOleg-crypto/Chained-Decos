#include "shadow_pass.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/scene/components/light_component.h"
#include "engine/scene/components/transform_component.h"
#include "engine/scene/entity.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Chained
{

void ShadowPass::Init()
{
    if (m_Initialized) return;

    // Create a depth-only framebuffer for the shadow map.
    FramebufferSpecification spec;
    spec.Width   = ShadowMapSize;
    spec.Height  = ShadowMapSize;
    spec.Samples = 1;
    m_ShadowMap  = Framebuffer::Create(spec);

    // Grab the depth-pass shader from the library if it exists.
    if (Renderer::GetShaderLibrary().Exists("ShadowDepth"))
    {
        m_DepthShaderAsset = Renderer::GetShaderLibrary().Get("ShadowDepth");
    }

    m_Initialized = true;
}

void ShadowPass::Execute(const RenderContext& ctx)
{
    if (!m_ShadowMap) return;

    // Build light-space matrix from the primary directional light direction, or use fallback.
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.5f));
    ctx.Registry.view<LightComponent>().each([&](LightComponent& lc) {
        if (lc.Type == LightType::Directional)
        {
            const auto& ld = ctx.Renderer->GetEnvironment().Lighting;
            lightDir = glm::normalize(glm::vec3(ld.Direction));
        }
    });

    constexpr float orthoSize = 50.0f;
    constexpr float nearPlane = 1.0f;
    constexpr float farPlane  = 200.0f;

    glm::mat4 lightProjection  = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
    glm::vec3 lightPos         = -lightDir * (farPlane * 0.5f);
    glm::mat4 lightView        = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_LightSpaceMatrix         = lightProjection * lightView;

    // Skip draw calls if no depth shader is loaded yet.
    if (!m_DepthShaderAsset || !m_DepthShaderAsset->GetShader()) return;

    auto* shader = m_DepthShaderAsset->GetShader().get();
    shader->Bind();
    shader->SetMatrix("u_LightSpaceMatrix", m_LightSpaceMatrix);

    m_ShadowMap->Bind();
    RenderCommand::SetViewport(0, 0, ShadowMapSize, ShadowMapSize);
    RenderCommand::Clear({0, 0, 0, 255});

    // Render all opaque items into the depth buffer using the depth shader.
    for (const auto& item : ctx.Renderer->GetOpaqueQueue())
    {
        ctx.Renderer->DrawModel(item.Asset, item.Transform,
                                item.BoneMatrices, item.Materials,
                                nullptr, {}, RenderPassStage::Opaque);
    }

    m_ShadowMap->Unbind();
}

void ShadowPass::Shutdown()
{
    m_ShadowMap.reset();
    m_DepthShaderAsset.reset();
    m_Initialized = false;
}

} // namespace Chained
