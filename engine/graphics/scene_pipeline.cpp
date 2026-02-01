#include "scene_pipeline.h"
#include "api_context.h"
#include "draw_command.h"
#include "engine/scene/components.h"

namespace CHEngine
{
    void ScenePipeline::Render(Scene *scene, const Camera3D &camera, const DebugRenderFlags *debugFlags)
    {
        // 0. Clear Background based on scene settings
        if (scene->GetBackgroundMode() == BackgroundMode::Color)
        {
            DrawCommand::Clear(scene->GetBackgroundColor());
        }
        else
        {
            // Default to black clear for 3D environments (skybox will overdraw it)
            DrawCommand::Clear(BLACK);
        }

        // 1. Setup Environment
        APIContext::ApplyEnvironment(scene->GetEnvironmentSettings());

        // 2. Render Passes
        ::BeginMode3D(camera);

        RenderSkybox(scene, camera);
        RenderModels(scene);

        if (debugFlags && debugFlags->IsAnyEnabled())
            RenderDebug(scene, debugFlags);

        // 3. Render Editor Icons (Billboards)
        RenderEditorIcons(scene, camera);

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
        if (!debugFlags) return;

        if (debugFlags->DrawGrid && scene->GetBackgroundMode() != BackgroundMode::Color)
        {
            DrawCommand::DrawGrid(999999, 1.0f); // Yea , I know , it's a bit much , but it works
        }

        if (debugFlags->DrawSpawnZones)
        {
            auto view = scene->GetRegistry().view<TransformComponent, SpawnComponent>();
            for (auto entity : view)
            {
                auto &tc = view.get<TransformComponent>(entity);
                auto &sc = view.get<SpawnComponent>(entity);

                if (sc.RenderSpawnZoneInScene)
                {
                    // Draw a semi-transparent box for the spawn zone
                    DrawCubeWires(tc.Translation, sc.ZoneSize.x, sc.ZoneSize.y, sc.ZoneSize.z, GREEN);
                    DrawCubeV(tc.Translation, sc.ZoneSize, Fade(GREEN, 0.2f));
                    
                    // Draw a line to the actual spawn point if offset
                    if (Vector3Length(sc.SpawnPoint) > 0.001f)
                    {
                        Vector3 worldSpawn = Vector3Add(tc.Translation, sc.SpawnPoint);
                        DrawLine3D(tc.Translation, worldSpawn, YELLOW);
                        DrawSphere(worldSpawn, 0.1f, YELLOW);
                    }
                }
            }
        }
    }

    void ScenePipeline::RenderEditorIcons(Scene *scene, const Camera3D &camera)
    {
        // Use hazel-style billboards or spheres for editor-only visualization
        
        // 1. Draw entities with explicit BillboardComponent
        auto billboardView = scene->GetRegistry().view<TransformComponent, BillboardComponent>();
        for (auto entityID : billboardView)
        {
            auto &tc = billboardView.get<TransformComponent>(entityID);
            auto &bc = billboardView.get<BillboardComponent>(entityID);
            if (!bc.TexturePath.empty())
            {
                DrawBillboard(camera, {0}, tc.Translation, bc.Size, bc.Tint);
            }
        }

        // 2. Draw Camera Icons (Hazel-style fallback)
        auto cameraView = scene->GetRegistry().view<TransformComponent, CameraComponent>();
        for (auto entityID : cameraView)
        {
            auto &tc = cameraView.get<TransformComponent>(entityID);
            if (!scene->GetRegistry().all_of<BillboardComponent>(entityID))
            {
                // Draw a small camera-like shape or icon
                DrawCube(tc.Translation, 0.4f, 0.2f, 0.2f, GRAY);
                DrawCube(Vector3Add(tc.Translation, {0, 0, 0.2f}), 0.1f, 0.1f, 0.1f, DARKGRAY);
            }
        }

        // 3. Draw Light Icons (Hazel-style fallback)
        auto lightView = scene->GetRegistry().view<TransformComponent, PointLightComponent>();
        for (auto entityID : lightView)
        {
            auto &tc = lightView.get<TransformComponent>(entityID);
            auto &plc = lightView.get<PointLightComponent>(entityID);
            if (!scene->GetRegistry().all_of<BillboardComponent>(entityID))
            {
                // Light bulb sphere with a "glow"
                DrawSphere(tc.Translation, 0.15f, plc.LightColor);
                DrawSphereEx(tc.Translation, 0.25f, 8, 8, { plc.LightColor.r, plc.LightColor.g, plc.LightColor.b, 50 });
            }
        }
    }
} // namespace CHEngine
