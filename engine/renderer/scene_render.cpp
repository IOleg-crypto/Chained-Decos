#include "scene_render.h"
#include "engine/renderer/asset_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "render.h"
#include "render_command.h"
#include <raymath.h>
#include <rlgl.h>

namespace CHEngine
{
struct SceneRenderData
{
    Scene *ActiveScene = nullptr;
    Camera3D SceneCamera;
};

static SceneRenderData s_Data;

// Forward declarations of helper methods
static void DrawColliderDebug(Scene *scene);
static void DrawLightDebug(Scene *scene);
static void DrawSpawnZoneDebug(Scene *scene);

void SceneRender::Init()
{
}

void SceneRender::Shutdown()
{
}

void SceneRender::BeginScene(Scene *scene, const Camera3D &camera)
{
    s_Data.ActiveScene = scene;
    s_Data.SceneCamera = camera;

    Render::BeginScene(camera);
}

void SceneRender::EndScene()
{
    Render::EndScene();
    s_Data.ActiveScene = nullptr;
}

void SceneRender::SubmitScene(Scene *scene, const DebugRenderFlags *debugFlags)
{
    if (!scene)
        return;

    // 0. Environment Pass
    Render::DrawSkybox(scene->GetSkybox(), s_Data.SceneCamera);

    // 1. Lighting Pass (Simple implementation for now)
    {
        auto view = scene->GetRegistry().view<TransformComponent, PointLightComponent>();
        int lightCount = 0;
        for (auto entity : view)
        {
            if (lightCount >= 8)
                break;
            auto [transform, light] = view.get<TransformComponent, PointLightComponent>(entity);
            // Renderer::SubmitLight(transform.Translation, light); // In future
            lightCount++;
        }
    }

    // 2. Opaque Pass
    RenderOpaquePass();

    // 3. Debug Pass
    if (debugFlags && debugFlags->IsAnyEnabled())
    {
        if (debugFlags->DrawColliders)
            DrawColliderDebug(scene);
        if (debugFlags->DrawLights)
            DrawLightDebug(scene);
        if (debugFlags->DrawSpawnZones)
            DrawSpawnZoneDebug(scene);
    }
}

void SceneRender::RenderOpaquePass()
{
    auto &registry = s_Data.ActiveScene->GetRegistry();

    auto view = registry.view<TransformComponent, ModelComponent>();
    for (auto entity : view)
    {
        auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);

        if (registry.all_of<MaterialComponent>(entity))
        {
            auto &mc = registry.get<MaterialComponent>(entity);
            Render::DrawModel(model.ModelPath, transform.GetTransform(), mc.Slots, model.Scale);
        }
        else
        {
            Render::DrawModel(model.ModelPath, transform.GetTransform(), WHITE, model.Scale);
        }
    }
}

void SceneRender::RenderTransparentPass()
{
}

static void DrawColliderDebug(Scene *scene)
{
    auto view = scene->GetRegistry().view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);

        Vector3 scale = transform.Scale;
        Vector3 scaledSize = Vector3Multiply(collider.Size, scale);
        Vector3 scaledOffset = Vector3Multiply(collider.Offset, scale);

        Vector3 min = Vector3Add(transform.Translation, scaledOffset);
        Vector3 center = Vector3Add(min, Vector3Scale(scaledSize, 0.5f));

        Color color;
        if (!collider.bEnabled)
            color = GRAY;
        else if (collider.IsColliding)
            color = RED;
        else if (collider.Type == ColliderType::Mesh)
            color = SKYBLUE;
        else
            color = GREEN;

        if (collider.Type == ColliderType::Mesh && !collider.ModelPath.empty())
        {
            auto asset = Assets::LoadModel(collider.ModelPath);
            if (asset)
            {
                Model &model = asset->GetModel();
                Matrix originalTransform = model.transform;
                model.transform = MatrixMultiply(originalTransform, transform.GetTransform());
                ::DrawModelWires(model, {0, 0, 0}, 1.0f, color);
                model.transform = originalTransform;
            }
        }
        else
        {
            ::DrawCubeWires(center, scaledSize.x, scaledSize.y, scaledSize.z, color);
        }
    }
}

static void DrawLightDebug(Scene *scene)
{
    auto view = scene->GetRegistry().view<TransformComponent, PointLightComponent>();
    for (auto entity : view)
    {
        auto [transform, light] = view.get<TransformComponent, PointLightComponent>(entity);
        ::DrawSphere(transform.Translation, 0.2f, light.LightColor);
        ::DrawSphereWires(transform.Translation, light.Radius, 16, 16,
                          ColorAlpha(light.LightColor, 0.3f));
    }
}

static void DrawSpawnZoneDebug(Scene *scene)
{
    auto view = scene->GetRegistry().view<TransformComponent, SpawnComponent>();
    for (auto entity : view)
    {
        auto [transform, spawn] = view.get<TransformComponent, SpawnComponent>(entity);
        Vector3 pos = transform.Translation;
        Vector3 size = spawn.ZoneSize;

        ::DrawCubeWires(pos, size.x, size.y, size.z, Color{0, 255, 255, 255});
        ::DrawCube(pos, size.x, size.y, size.z, ColorAlpha(Color{0, 255, 255, 255}, 0.2f));
    }
}

} // namespace CHEngine
