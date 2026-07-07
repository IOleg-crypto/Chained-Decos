#include "composite_pass.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/scene_renderer.h"

namespace Chained
{

void CompositePass::Init()
{
    m_Initialized = true;
}

void CompositePass::Execute(const RenderContext& ctx)
{
    if (!m_Initialized) return;

    const auto& env = ctx.Renderer->GetEnvironment();

    // Upload fog uniforms to the lighting shader so subsequent draws
    // evaluate the same fog parameters as the main geometry pass.
    if (!ServiceLocator::Get<Renderer>()->GetShaderLibrary().Exists("Lighting")) return;

    auto lightingAsset = ServiceLocator::Get<Renderer>()->GetShaderLibrary().Get("Lighting");
    if (!lightingAsset) return;

    auto shader = lightingAsset->GetShader();
    if (!shader)  return;

    shader->Bind();
    shader->SetInt ("fogEnabled",   env.Fog.Enabled ? 1 : 0);
    shader->SetVec4("fogColor",
        { env.Fog.FogColor.r / 255.0f,
          env.Fog.FogColor.g / 255.0f,
          env.Fog.FogColor.b / 255.0f,
          env.Fog.FogColor.a / 255.0f });
    shader->SetFloat("fogDensity",  env.Fog.Density);
    shader->SetFloat("fogStart",    env.Fog.Start);
    shader->SetFloat("fogEnd",      env.Fog.End);
    shader->SetInt  ("fogMode",     (int)env.Fog.Mode);
    shader->SetFloat("fogHeightFalloff", env.Fog.HeightFalloff);

    // Future: full-screen HDR blit with tone-mapping when m_HDRTarget is set.
}

void CompositePass::Shutdown()
{
    m_HDRTarget.reset();
    m_Initialized = false;
}

} // namespace Chained
