#include "shadow_pass.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/project/project.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Chained
{

void ShadowPass::Init()
{
    if (m_Initialized) return;

    // Grab the depth-pass shader from the library if it exists.
    if (ServiceLocator::Get<Renderer>()->GetShaderLibrary().Exists("ShadowDepth"))
    {
        m_DepthShaderAsset = ServiceLocator::Get<Renderer>()->GetShaderLibrary().Get("ShadowDepth");
    }

    m_Initialized = true;
}

void ShadowPass::Execute(const RenderContext& ctx)
{
    // Always cast shadows from the global environment directional light,
    // unless the project's Render settings have shadows disabled entirely.
    auto project = Project::GetActive();
    if (project && !project->GetConfig().Render.EnableShadows)
    {
        m_HasShadows = false;
        return;
    }

    const auto& ld = ctx.Renderer->GetEnvironment().Lighting;
    glm::vec3 lightDir = glm::length(glm::vec3(ld.Direction)) > 0.0001f
                             ? glm::normalize(glm::vec3(ld.Direction))
                             : glm::normalize(glm::vec3(0.3f, -0.7f, 0.3f));

    m_HasShadows = true;
    if (!m_DepthShaderAsset || !m_DepthShaderAsset->GetShader()) return;

    // Read shadow resolution from project settings
    uint32_t shadowRes = 2048;
    if (project)
    {
        shadowRes = (uint32_t)project->GetConfig().Render.ShadowResolution;
        if (shadowRes == 0) shadowRes = 2048;
    }
    m_ShadowMapSize = shadowRes;

    // Recreate shadow map FBO if size changed (depth-only)
    if (!m_ShadowMap || m_ShadowMap->GetSpecification().Width != shadowRes)
    {
        FramebufferSpecification spec;
        spec.Width     = shadowRes;
        spec.Height    = shadowRes;
        spec.Samples   = 1;
        spec.DepthOnly = true;  // Pure depth texture — no color attachment
        m_ShadowMap    = Framebuffer::Create(spec);
    }

    if (!m_ShadowMap || !m_ShadowMap->IsValid()) return;

    // Build light-space matrix
    constexpr float orthoSize = 80.0f;
    constexpr float nearPlane = 1.0f;
    constexpr float farPlane  = 300.0f;

    glm::mat4 lightProjection  = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
    glm::vec3 lightPos         = -lightDir * (farPlane * 0.5f);
    glm::mat4 lightView        = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_LightSpaceMatrix         = lightProjection * lightView;

    // Set shadow bias on renderer
    ServiceLocator::Get<Renderer>()->GetData().ShadowBias = 0.003f;

    auto* shader = m_DepthShaderAsset->GetShader().get();
    shader->Bind();
    shader->SetMatrix("u_LightSpaceMatrix", m_LightSpaceMatrix);

    // Save current FBO binding
    GLint previousFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);

    m_ShadowMap->Bind();
    GraphicsDevice::Get().SetViewport(0, 0, shadowRes, shadowRes);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Depth bias to prevent shadow acne (surface-shadow self-intersection)
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 1.0f);

    // Render all opaque items into the depth buffer using the depth shader.
    for (const auto& item : ctx.Renderer->GetOpaqueQueue())
    {
        ctx.Renderer->DrawModel(item.Asset, item.Transform,
                                item.BoneMatrices, item.Materials,
                                m_DepthShaderAsset.get(), {}, RenderPassStage::Opaque);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);

    // Restore previous FBO binding
    m_ShadowMap->Unbind();
    glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
}

void ShadowPass::Shutdown()
{
    m_ShadowMap.reset();
    m_DepthShaderAsset.reset();
    m_Initialized = false;
}

} // namespace Chained
