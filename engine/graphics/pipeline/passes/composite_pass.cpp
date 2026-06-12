#include "composite_pass.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/pipeline/renderer.h"
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
    if (!Renderer::GetShaderLibrary().Exists("Lighting")) return;

    auto lightingAsset = Renderer::GetShaderLibrary().Get("Lighting");
    if (!lightingAsset) return;

    auto shader = lightingAsset->GetShader();
    if (!shader)  return;

    shader->Bind();
    shader->SetInt ("u_FogEnabled",  env.Fog.Enabled ? 1 : 0);
    shader->SetVec4("u_FogColor",
        { env.Fog.FogColor.r / 255.0f,
          env.Fog.FogColor.g / 255.0f,
          env.Fog.FogColor.b / 255.0f,
          env.Fog.FogColor.a / 255.0f });
    shader->SetFloat("u_FogDensity", env.Fog.Density);
    shader->SetFloat("u_FogStart",   env.Fog.Start);
    shader->SetFloat("u_FogEnd",     env.Fog.End);

    // Future: full-screen HDR blit with tone-mapping when m_HDRTarget is set.
}

void CompositePass::Shutdown()
{
    m_HDRTarget.reset();
    m_Initialized = false;
}

} // namespace Chained
