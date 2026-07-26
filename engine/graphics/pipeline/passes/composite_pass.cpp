#include "composite_pass.h"
#include "engine/core/service_locator.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/pipeline/renderer.h"

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

    // Delegate fog uniform upload to LightingManager to avoid duplication.
    if (!ServiceLocator::Get<Renderer>()->GetShaderLibrary().Exists("Lighting")) return;

    auto lightingAsset = ServiceLocator::Get<Renderer>()->GetShaderLibrary().Get("Lighting");
    if (!lightingAsset) return;

    auto shader = lightingAsset->GetShader();
    if (!shader) return;

    shader->Bind();
    ServiceLocator::Get<Renderer>()->ApplyFogUniforms(lightingAsset.get());

    // Future: full-screen HDR blit with tone-mapping when m_HDRTarget is set.
}

void CompositePass::Shutdown()
{
    m_HDRTarget.reset();
    m_Initialized = false;
}

} // namespace Chained
