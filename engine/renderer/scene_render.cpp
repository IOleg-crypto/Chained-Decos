#include "scene_render.h"
#include "engine/core/profiler.h"
#include "engine/renderer/asset_manager.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "render.h"
#include <raymath.h>
#include <rlgl.h>

namespace CHEngine
{
// SceneRender data is usually kept static in the translation unit
struct SceneRenderData
{
    Camera3D SceneCamera;
};

static SceneRenderData s_Data;

void SceneRender::Init()
{
}

void SceneRender::Shutdown()
{
}

void SceneRender::BeginScene(const RenderState &state)
{
    s_Data.SceneCamera = state.SceneCamera;
    Render::BeginScene(state.SceneCamera);
}

void SceneRender::EndScene()
{
    Render::EndScene();
}

void SceneRender::SubmitScene(const RenderState &state)
{
    CH_PROFILE_FUNCTION();

    // 0. Environment Pass
    Render::ApplyEnvironment(state.Environment);
    Render::DrawSkybox(state.Environment, state.SceneCamera);

    // 1. Unified Command Pass
    for (const auto &cmd : state.Commands)
    {
        switch (cmd.Type)
        {
        case RenderPacketType::Mesh:
            if (cmd.Model)
                Render::DrawModel(cmd.Model, cmd.Transform, cmd.MaterialSlots);
            break;
        case RenderPacketType::PointLight:
            // Point lights are usually handled by SetShaderValues, but here we might draw debug
            // gizmos or queue them for a light manager.
            break;
        case RenderPacketType::Skybox:
            // Handled in environment pass
            break;
        case RenderPacketType::DebugBox:
            ::DrawCubeWires(cmd.Position, cmd.Size.x, cmd.Size.y, cmd.Size.z, cmd.Tint);
            break;
        case RenderPacketType::DebugSphere:
            ::DrawSphereWires(cmd.Position, cmd.Radius, 16, 16, cmd.Tint);
            break;
        case RenderPacketType::DebugLine:
            // TODO: Add Start/End to RenderPacket
            break;
        case RenderPacketType::DebugMeshWires:
            if (!cmd.Metadata.empty())
            {
                auto asset = Assets::Get<ModelAsset>(cmd.Metadata);
                if (asset)
                {
                    Model &model = asset->GetModel();
                    Matrix originalTransform = model.transform;
                    model.transform = MatrixMultiply(originalTransform, cmd.Transform);
                    ::DrawModelWires(model, {0, 0, 0}, 1.0f, cmd.Tint);
                    model.transform = originalTransform;
                }
            }
            break;
        }
    }
}

void SceneRender::CreateSnapshot(Scene *scene, const Camera3D &camera, RenderState &outState,
                                 float alpha, const DebugRenderFlags *debugFlags)
{
    CH_PROFILE_FUNCTION();
    outState.Clear();
    outState.SceneCamera = camera;
    outState.Alpha = alpha;

    auto envAsset = scene->GetEnvironment();
    if (envAsset)
        outState.Environment = envAsset->GetSettings();
    else
    {
        auto &sky = scene->GetSkybox();
        outState.Environment.Skybox.TexturePath = sky.TexturePath;
        outState.Environment.Skybox.Exposure = sky.Exposure;
        outState.Environment.Skybox.Brightness = sky.Brightness;
        outState.Environment.Skybox.Contrast = sky.Contrast;
    }

    auto &registry = scene->GetRegistry();

    // 1. Entities (Meshes)
    auto view = registry.view<TransformComponent, ModelComponent>();
    for (auto entity : view)
    {
        auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);

        RenderPacket pkt;
        pkt.Type = RenderPacketType::Mesh;
        pkt.Transform = transform.GetInterpolatedTransform(alpha);

        if (!model.Asset && !model.ModelPath.empty())
            model.Asset = Assets::Get<ModelAsset>(model.ModelPath);

        pkt.Model = model.Asset;

        pkt.MaterialSlots = model.Materials;

        outState.Commands.push_back(pkt);
    }

    // Update Statistics
    ProfilerStats stats;
    stats.EntityCount = (uint32_t)registry.storage<entt::entity>().size();
    Profiler::UpdateStats(stats);

    // 2. Lights (Currently point lights are set directly in Render::ApplyEnvironment,
    // but we can snapshot them for forward rendering passes)
    auto lightView = registry.view<TransformComponent, PointLightComponent>();
    for (auto entity : lightView)
    {
        auto [transform, light] = lightView.get<TransformComponent, PointLightComponent>(entity);
        RenderPacket pkt;
        pkt.Type = RenderPacketType::PointLight;
        pkt.Position = transform.Translation;
        pkt.Radius = light.Radius;
        pkt.Radiance = light.Radiance;
        pkt.Tint = light.LightColor;
        outState.Commands.push_back(pkt);
    }

    // 3. Debug Data
    if (debugFlags && debugFlags->IsAnyEnabled())
    {
        if (debugFlags->DrawColliders)
        {
            auto colView = registry.view<TransformComponent, ColliderComponent>();
            for (auto entity : colView)
            {
                auto [transform, collider] =
                    colView.get<TransformComponent, ColliderComponent>(entity);

                RenderPacket pkt;
                pkt.Transform = transform.GetTransform();

                Vector3 scale = transform.Scale;
                Vector3 scaledSize = Vector3Multiply(collider.Size, scale);
                Vector3 scaledOffset = Vector3Multiply(collider.Offset, scale);

                // Raylib's DrawCubeWires expects the CENTER of the box.
                // Our 'scaledOffset' is relative to the min corner.
                Vector3 minCorner = Vector3Add(transform.Translation, scaledOffset);
                pkt.Position = Vector3Add(minCorner, Vector3Scale(scaledSize, 0.5f));
                pkt.Size = scaledSize;

                if (!collider.bEnabled)
                    pkt.Tint = GRAY;
                else if (collider.IsColliding)
                    pkt.Tint = RED;
                else
                    pkt.Tint = GREEN;

                if (collider.Type == ColliderType::Mesh)
                {
                    pkt.Type = RenderPacketType::DebugMeshWires;
                    pkt.Metadata = collider.ModelPath;
                    if (collider.bEnabled && !collider.IsColliding)
                        pkt.Tint = SKYBLUE;
                }
                else
                {
                    pkt.Type = RenderPacketType::DebugBox;
                    // DrawCube expects center and size.
                    // Raylib's DrawCubeWires takes (center, width, height, length)
                    // Our 'Position' is the center.
                }

                outState.Commands.push_back(pkt);
            }
        }

        if (debugFlags->DrawLights)
        {
            // Already handled in lights pass above if we want gizmos
            for (auto entity : lightView)
            {
                auto [transform, light] =
                    lightView.get<TransformComponent, PointLightComponent>(entity);
                RenderPacket pkt;
                pkt.Type = RenderPacketType::DebugSphere;
                pkt.Position = transform.Translation;
                pkt.Radius = 0.2f;
                pkt.Tint = light.LightColor;
                outState.Commands.push_back(pkt);

                RenderPacket rangePkt;
                rangePkt.Type = RenderPacketType::DebugSphere;
                rangePkt.Position = transform.Translation;
                rangePkt.Radius = light.Radius;
                rangePkt.Tint = ColorAlpha(light.LightColor, 0.3f);
                outState.Commands.push_back(rangePkt);
            }
        }

        if (debugFlags->DrawSpawnZones)
        {
            auto spawnView = registry.view<TransformComponent, SpawnComponent>();
            for (auto entity : spawnView)
            {
                auto [transform, spawn] = spawnView.get<TransformComponent, SpawnComponent>(entity);
                RenderPacket pkt;
                pkt.Type = RenderPacketType::DebugBox;
                pkt.Position = transform.Translation;
                pkt.Size = spawn.ZoneSize;
                pkt.Tint = {0, 255, 255, 255};
                outState.Commands.push_back(pkt);
            }
        }
    }
}

// Legacy implementations
void SceneRender::RenderOpaquePass()
{
}
void SceneRender::RenderTransparentPass()
{
}
void SceneRender::RenderDebugPass()
{
}

} // namespace CHEngine
