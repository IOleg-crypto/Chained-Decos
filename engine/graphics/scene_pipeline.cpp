#include "scene_pipeline.h"
#include "api_context.h"
#include "draw_command.h"
#include "engine/scene/components.h"

namespace CHEngine
{
    void ScenePipeline::Render(Scene *scene, const Camera3D &camera, const DebugRenderFlags *debugFlags)
    {
        // 1. Setup Environment
        APIContext::ApplyEnvironment(scene->GetEnvironmentSettings());

        // 2. Render Passes
        ::BeginMode3D(camera);

        RenderSkybox(scene, camera);
        RenderModels(scene);

        if (debugFlags && debugFlags->IsAnyEnabled())
            RenderDebug(scene, debugFlags);

        ::EndMode3D();
    }

    void ScenePipeline::RenderSkybox(Scene *scene, const Camera3D &camera)
    {
        // Use Global Scene Skybox settings
        DrawCommand::DrawSkybox(scene->GetSkybox(), camera);
    }

    void ScenePipeline::RenderModels(Scene *scene)
    {
        auto view = scene->GetRegistry().view<TransformComponent, ModelComponent>();
        for (auto entity : view)
        {
            auto &transform = view.get<TransformComponent>(entity);
            auto &model = view.get<ModelComponent>(entity);

            if (!model.ModelPath.empty())
            {
                DrawCommand::DrawModel(model.ModelPath, transform.GetTransform(), model.Materials);
            }
        }
    }

    void ScenePipeline::RenderDebug(Scene *scene, const DebugRenderFlags *debugFlags)
    {
        if (debugFlags && debugFlags->DrawGrid)
        {
            DrawCommand::DrawGrid(20, 1.0f);
        }

        // Draw World Axes
        if (debugFlags->DrawAxes)
        {
            DrawCommand::DrawLine({0, 0, 0}, {5, 0, 0}, RED);   // X
            DrawCommand::DrawLine({0, 0, 0}, {0, 5, 0}, GREEN); // Y
            DrawCommand::DrawLine({0, 0, 0}, {0, 0, 5}, BLUE);  // Z
        }
    }
} // namespace CHEngine
