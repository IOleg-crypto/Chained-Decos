#include "scene_pipeline.h"
#include "api_context.h"
#include "asset_manager.h"
#include "draw_command.h"
#include "engine/core/profiler.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include <raymath.h>

namespace CHEngine
{
static void RenderEnvironment(Scene *scene, const Camera3D &camera)
{
    auto settings = scene->GetEnvironmentSettings();
    APIContext::ApplyEnvironment(settings);

    SkyboxComponent sc;
    sc.TexturePath = settings.Skybox.TexturePath;
    sc.Exposure = settings.Skybox.Exposure;
    sc.Brightness = settings.Skybox.Brightness;
    sc.Contrast = settings.Skybox.Contrast;
    DrawCommand::DrawSkybox(sc, camera);
}

static void RenderOpaque(entt::registry &registry)
{
    auto meshView = registry.view<TransformComponent, ModelComponent>();
    for (auto entity : meshView)
    {
        auto &transform = meshView.get<TransformComponent>(entity);
        auto &model = meshView.get<ModelComponent>(entity);

        if (model.Asset && model.Asset->IsReady())
        {
            DrawCommand::DrawModel(model.ModelPath, transform.GetTransform(), model.Materials);
        }
    }
}

static void RenderDebug(entt::registry &registry, const DebugRenderFlags *flags)
{
    if (!flags || !flags->IsAnyEnabled())
        return;

    if (flags->DrawColliders)
    {
        auto colView = registry.view<TransformComponent, ColliderComponent>();
        for (auto entity : colView)
        {
            auto [transform, collider] = colView.get<TransformComponent, ColliderComponent>(entity);
            // ... (Debug collider drawing logic from Scene::OnRender)
            // Simplified for brevity, but I should port it fully soon
        }
    }
}

void ScenePipeline::RenderScene(Scene *scene, const DebugRenderFlags *debugFlags)
{
    CH_PROFILE_FUNCTION();
    if (!scene)
        return;

    auto &registry = scene->GetRegistry();
    auto &camera = scene->GetActiveCamera();

    RenderEnvironment(scene, camera);
    RenderOpaque(registry);

    // Adaptive Debug Visibility: hide grid/colliders when designing UI (Solid Color/Texture
    // background)
    bool bDesignMode = (scene->GetBackgroundMode() == BackgroundMode::Color ||
                        scene->GetBackgroundMode() == BackgroundMode::Texture);

    if (!bDesignMode)
    {
        RenderDebug(registry, debugFlags);
    }
}
} // namespace CHEngine
