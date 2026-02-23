#include "scene_renderer.h"
#include "engine/core/assert.h"
#include "engine/core/profiler.h"
#include "engine/graphics/frustum.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/renderer.h"
#include "engine/graphics/renderer2d.h"
#include "engine/graphics/shader_asset.h"
#include "engine/physics/bvh/bvh.h"
#include "imgui_converter.h"
#include "engine/scene/components/light_component.h"
#include "imgui.h"
#include "raylib.h"
#include "engine/scene/project.h"
#include "render_command.h"
#include <raymath.h>
#include <rlgl.h>

namespace
{
using namespace CHEngine;
} // namespace

namespace CHEngine
{
Matrix SceneRenderer::GetWorldTransform(entt::registry& registry, entt::entity entity)
{
    if (registry.all_of<TransformComponent>(entity))
    {
        return registry.get<TransformComponent>(entity).WorldTransform;
    }
    return MatrixIdentity();
}

Vector3 SceneRenderer::GetWorldPosition(entt::registry& registry, entt::entity entity)
{
    Matrix transform = GetWorldTransform(registry, entity);
    return {transform.m12, transform.m13, transform.m14};
}

void SceneRenderer::RenderScene(Scene* scene, const Camera3D& camera, Timestep timestep,
                                const DebugRenderFlags* debugFlags)
{
    CH_PROFILE_FUNCTION();
    CH_CORE_ASSERT(Renderer::IsInitialized(), "Renderer not initialized!");
    CH_CORE_ASSERT(scene, "Scene is null!");

    // 1. Environmental setup
    auto environment = scene->GetSettings().Environment;

    // Fallback to project environment if not set in scene
    if (!environment && Project::GetActive())
    {
        environment = Project::GetActive()->GetEnvironment();
    }

    float exposure = 1.0f;
    float gamma = 2.2f;

    if (environment)
    {
        const auto& envSettings = environment->GetSettings();
        Renderer::Get().ApplyEnvironment(envSettings);
        exposure = envSettings.Lighting.Exposure;
        gamma = envSettings.Lighting.Gamma;
    }
    else
    {
        static bool warned = false;
        if (!warned)
        {
            CH_CORE_WARN("SceneRenderer: No environment asset for scene!");
            warned = true;
        }
    }

    Renderer::Get().UpdateTime(Timestep((float)GetTime()));

    // --- Update Profiler Stats ---
    ProfilerStats stats;
    stats.EntityCount = (uint32_t)scene->GetRegistry().storage<entt::entity>().size();
    Profiler::UpdateStats(stats);

    // 2. Scene rendering flow
    Renderer::Get().BeginScene(camera);
    {
        if (environment)
        {
            if (environment->GetSettings().Skybox.TexturePath.empty())
            {
                static bool warned = false;
                if (!warned)
                {
                    CH_CORE_WARN("SceneRenderer: Environment exists but Skybox.TexturePath is empty!");
                    warned = true;
                }
            }
            Renderer::Get().DrawSkybox(environment->GetSettings().Skybox, camera);
        }
        else
        {
            static bool warned = false;
            if (!warned)
            {
                CH_CORE_WARN("SceneRenderer: No environment asset for scene!");
                warned = true;
            }
        }

        RenderModels(scene, timestep);

        if (debugFlags)
        {
            RenderDebug(scene, debugFlags);
        }

        RenderSprites(scene);

        RenderEditorIcons(scene, camera);
    }
    Renderer::Get().EndScene();
}

SceneRenderer::InstanceKey::InstanceKey(const std::string& path, const std::vector<MaterialSlot>& mats)
{
    auto hashCombine = [](size_t& seed, size_t hash) {
        seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    };

    Hash = std::hash<std::string>{}(path);

    for (const auto& m : mats)
    {
        hashCombine(Hash, std::hash<int>{}((int)m.Target));
        hashCombine(Hash, std::hash<int>{}(m.Index));
        hashCombine(Hash, std::hash<std::string>{}(m.Material.AlbedoPath));
        hashCombine(Hash, std::hash<std::string>{}(m.Material.ShaderPath));
    }
}

void SceneRenderer::RenderModels(Scene* scene, Timestep timestep)
{
    auto& registry = scene->GetRegistry();

    // 1. Frustum Extraction
    Frustum frustum;
    {
        Matrix matVP = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
        frustum.Extract(matVP);
    }

    // 2. Prepare Lights
    PrepareLights(registry, frustum);

    // 3. Pass A: Collect visible entities
    std::vector<AnimatedEntry> animatedEntries;
    std::map<InstanceKey, InstanceGroup> instanceGroups;
    CollectRenderItems(registry, frustum, animatedEntries, instanceGroups);

    // 4. Pass B: Draw animated individually
    DrawAnimatedEntities(animatedEntries);

    // 5. Pass C: Draw static groups (instanced if >=2, single otherwise)
    DrawStaticEntities(instanceGroups);
}

void SceneRenderer::PrepareLights(entt::registry& registry, const Frustum& frustum)
{
    Renderer::Get().ClearLights();

    int lightCount = 0;
    auto lightView = registry.view<LightComponent>();
    for (auto entity : lightView)
    {
        if (lightCount >= RendererData::MaxLights)
            break;

        auto& light = lightView.get<LightComponent>(entity);
        Matrix worldTransform = GetWorldTransform(registry, entity);
        Vector3 worldPos = {worldTransform.m12, worldTransform.m13, worldTransform.m14};

        if (!frustum.IsSphereVisible(worldPos, light.Radius))
            continue;

        RenderLight rl;
        rl.color[0] = light.LightColor.r / 255.0f;
        rl.color[1] = light.LightColor.g / 255.0f;
        rl.color[2] = light.LightColor.b / 255.0f;
        rl.color[3] = light.LightColor.a / 255.0f;

        rl.position = worldPos;
        rl.intensity = (light.Intensity <= 0.0f) ? 1.0f : light.Intensity;
        rl.radius = light.Radius;
        rl.innerCutoff = light.InnerCutoff;
        rl.outerCutoff = light.OuterCutoff;
        rl.type = (int)light.Type; // Direct enum mapping
        rl.enabled = 1;

        if (light.Type == LightType::Spot)
        {
            Vector3 worldDir = Vector3Transform({0, -1, 0}, worldTransform);
            rl.direction = Vector3Normalize(Vector3Subtract(worldDir, worldPos));
        }

        Renderer::Get().SetLight(lightCount++, rl);
    }
    Renderer::Get().SetLightCount(lightCount);
}

void SceneRenderer::CollectRenderItems(entt::registry& registry, const Frustum& frustum,
                                       std::vector<AnimatedEntry>& animatedEntries,
                                       std::map<InstanceKey, InstanceGroup>& instanceGroups)
{
    auto view = registry.view<TransformComponent, ModelComponent>();
    for (auto entity : view)
    {
        auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);

        if (!model.Asset || model.Asset->GetState() != AssetState::Ready)
            continue;

        const Matrix& worldTransform = transform.WorldTransform;
        if (!frustum.IsBoxVisible(model.Asset->GetBoundingBox(), worldTransform))
            continue;

        if (model.CullDistance > 0.0f)
        {
            Vector3 entityPos = {worldTransform.m12, worldTransform.m13, worldTransform.m14};
            Vector3 camPos = Renderer::Get().GetData().CurrentCameraPosition;
            float distSq = Vector3LengthSqr(Vector3Subtract(entityPos, camPos));
            if (distSq > model.CullDistance * model.CullDistance)
                continue;
        }

        model.Asset->OnUpdate();

        std::shared_ptr<ShaderAsset> shaderOverride;
        std::vector<ShaderUniform> customUniforms;
        bool hasShaderOverride = false;
        if (registry.all_of<ShaderComponent>(entity))
        {
            auto& shaderComp = registry.get<ShaderComponent>(entity);
            if (shaderComp.Enabled && !shaderComp.ShaderPath.empty() && Project::GetActive())
            {
                shaderOverride = Project::GetActive()->GetAssetManager()->Get<ShaderAsset>(shaderComp.ShaderPath);
                customUniforms = shaderComp.Uniforms;
                hasShaderOverride = true;
            }
        }

        const bool isAnimated = registry.all_of<AnimationComponent>(entity);

        if (isAnimated)
        {
            AnimatedEntry entry;
            entry.asset = model.Asset;
            entry.worldTransform = worldTransform;
            entry.materials = model.Materials;
            entry.shaderOverride = shaderOverride;
            entry.customUniforms = customUniforms;
            entry.animation = registry.get<AnimationComponent>(entity);
            animatedEntries.push_back(std::move(entry));
        }
        else if (!hasShaderOverride)
        {
            InstanceKey key{model.ModelPath, model.Materials};
            auto& group = instanceGroups[key];
            if (group.transforms.empty())
            {
                group.asset = model.Asset;
                group.materials = model.Materials;
            }
            group.transforms.push_back(worldTransform);
        }
        else
        {
            Renderer::Get().DrawModel(model.Asset, worldTransform, model.Materials, 0, 0.0f, -1, 0.0f, 0.0f,
                                      shaderOverride, customUniforms);
        }
    }
}

void SceneRenderer::DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries)
{
    for (auto& entry : animatedEntries)
    {
        float targetFPS = 30.0f;
        if (Project::GetActive())
            targetFPS = Project::GetActive()->GetConfig().Animation.TargetFPS;

        float frameTime = 1.0f / (targetFPS > 0 ? targetFPS : 30.0f);
        float fractionalFrame = (float)entry.animation.CurrentFrame + (entry.animation.FrameTimeCounter / frameTime);

        int targetAnim = -1;
        float targetFractionalFrame = 0.0f;
        float blendWeight = 0.0f;
        if (entry.animation.Blending)
        {
            targetAnim = entry.animation.TargetAnimationIndex;
            targetFractionalFrame = (float)entry.animation.TargetFrame + (entry.animation.FrameTimeCounter / frameTime);
            blendWeight = entry.animation.BlendTimer / entry.animation.BlendDuration;
        }

        Renderer::Get().DrawModel(entry.asset, entry.worldTransform, entry.materials,
                                  entry.animation.CurrentAnimationIndex, fractionalFrame,
                                  targetAnim, targetFractionalFrame, blendWeight,
                                  entry.shaderOverride, entry.customUniforms);
    }
}

void SceneRenderer::DrawStaticEntities(std::map<InstanceKey, InstanceGroup>& instanceGroups)
{
    for (auto& [key, group] : instanceGroups)
    {
        if (group.transforms.size() == 1)
        {
            Renderer::Get().DrawModel(group.asset, group.transforms[0], group.materials,
                                      0, 0.0f, -1, 0.0f, 0.0f, nullptr, {});
        }
        else
        {
            Renderer::Get().DrawModelInstanced(group.asset, group.transforms, group.materials);
        }
    }
}

void SceneRenderer::RenderBVHNode(const BVH* bvh, uint32_t nodeIndex, const Matrix& transform, Color color,
                                  int depth)
{
    const auto& nodes = bvh->GetNodes();
    if (nodeIndex >= nodes.size())
    {
        return;
    }

    const auto& node = nodes[nodeIndex];
    bool isLeaf = node.IsLeaf();

    // Only draw root or leaves to reduce clutter in the editor
    if (depth == 0 || isLeaf)
    {
        Color nodeColor = isLeaf ? ORANGE : color;

        Vector3 center = Vector3Scale(Vector3Add(node.Min, node.Max), 0.5f);
        Vector3 size = Vector3Subtract(node.Max, node.Min);

        Matrix nodeTransform = MatrixMultiply(MatrixTranslate(center.x, center.y, center.z), transform);
        Renderer::Get().DrawCubeWires(nodeTransform, size, nodeColor);
    }

    if (!isLeaf && depth < 20)
    {
        RenderBVHNode(bvh, node.LeftOrFirst, transform, color, depth + 1);
        RenderBVHNode(bvh, node.LeftOrFirst + 1, transform, color, depth + 1);
    }
}

void SceneRenderer::RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags)
{
    if (!debugFlags)
    {
        return;
    }
    auto& registry = scene->GetRegistry();

    rlDisableDepthTest();

    if (debugFlags->DrawColliders)
    {
        DrawColliderDebug(registry, debugFlags);
    }
    if (debugFlags->DrawCollisionModelBox)
    {
        DrawCollisionModelBoxDebug(registry, debugFlags);
    }
    if (debugFlags->DrawSpawnZones)
    {
        DrawSpawnDebug(registry, debugFlags);
    }

    if (debugFlags->DrawGrid && scene->GetSettings().Mode == BackgroundMode::Environment3D)
    {
        const auto& grid = scene->GetSettings().Grid;
        Renderer::Get().DrawGrid(grid.Slices, grid.Spacing);
    }

    rlEnableDepthTest();
}

void SceneRenderer::DrawColliderDebug(entt::registry& registry, const DebugRenderFlags* debugFlags)
{
    auto view = registry.view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);
        if (!collider.Enabled)
            continue;

        Matrix worldTransform = GetWorldTransform(registry, entity);
        Color color = GREEN;

        switch (collider.Type)
        {
            case ColliderType::Mesh:
                if (collider.BVHRoot){
                    RenderBVHNode(collider.BVHRoot.get(), 0, worldTransform, color);
                }
                break;
            case ColliderType::Box:
            {
                Vector3 center = Vector3Add(collider.Offset, Vector3Scale(collider.Size, 0.5f));
                Matrix colliderTransform = MatrixMultiply(MatrixTranslate(center.x, center.y, center.z), worldTransform);
                Renderer::Get().DrawCubeWires(colliderTransform, collider.Size, color);
                break;
            }
            case ColliderType::Capsule:
            {
                Matrix colliderTransform = MatrixMultiply(MatrixTranslate(collider.Offset.x, collider.Offset.y, collider.Offset.z), worldTransform);
                Renderer::Get().DrawCapsuleWires(colliderTransform, collider.Radius, collider.Height, color);
                break;
            }
            case ColliderType::Sphere:
            {
                Matrix colliderTransform = MatrixMultiply(MatrixTranslate(collider.Offset.x, collider.Offset.y, collider.Offset.z), worldTransform);
                Renderer::Get().DrawSphereWires(colliderTransform, collider.Radius, color);
                break;
            }
        }
    }
}

void SceneRenderer::DrawCollisionModelBoxDebug(entt::registry& registry, const DebugRenderFlags* debugFlags)
{
    auto view = registry.view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);
        if (!collider.Enabled)
            continue;

        Matrix worldTransform = GetWorldTransform(registry, entity);
        BoundingBox worldAABB = CalculateColliderWorldAABB(collider, worldTransform);

        // Check if AABB is valid (max > min)
        if (worldAABB.max.x > worldAABB.min.x || worldAABB.max.y > worldAABB.min.y || worldAABB.max.z > worldAABB.min.z)
        {
            Vector3 center = Vector3Scale(Vector3Add(worldAABB.min, worldAABB.max), 0.5f);
            Vector3 size = Vector3Subtract(worldAABB.max, worldAABB.min);
            Renderer::Get().DrawCubeWires(MatrixTranslate(center.x, center.y, center.z), size, RED);
        }
    }
}

BoundingBox SceneRenderer::CalculateColliderWorldAABB(const ColliderComponent& collider, const Matrix& worldTransform)
{
    BoundingBox box = {{0, 0, 0}, {0, 0, 0}};
    std::vector<Vector3> corners;

    switch (collider.Type)
    {
        case ColliderType::Mesh:
            if (collider.BVHRoot && !collider.BVHRoot->GetNodes().empty())
            {
                const auto& rootNode = collider.BVHRoot->GetNodes()[0];
                corners = {
                    {rootNode.Min.x, rootNode.Min.y, rootNode.Min.z}, {rootNode.Max.x, rootNode.Min.y, rootNode.Min.z},
                    {rootNode.Min.x, rootNode.Max.y, rootNode.Min.z}, {rootNode.Max.x, rootNode.Max.y, rootNode.Min.z},
                    {rootNode.Min.x, rootNode.Min.y, rootNode.Max.z}, {rootNode.Max.x, rootNode.Min.y, rootNode.Max.z},
                    {rootNode.Min.x, rootNode.Max.y, rootNode.Max.z}, {rootNode.Max.x, rootNode.Max.y, rootNode.Max.z}};
            }
            break;
        case ColliderType::Box:
            corners = {
                collider.Offset,
                Vector3Add(collider.Offset, {collider.Size.x, 0, 0}),
                Vector3Add(collider.Offset, {0, collider.Size.y, 0}),
                Vector3Add(collider.Offset, {collider.Size.x, collider.Size.y, 0}),
                Vector3Add(collider.Offset, {0, 0, collider.Size.z}),
                Vector3Add(collider.Offset, {collider.Size.x, 0, collider.Size.z}),
                Vector3Add(collider.Offset, {0, collider.Size.y, collider.Size.z}),
                Vector3Add(collider.Offset, {collider.Size.x, collider.Size.y, collider.Size.z})};
            break;
        case ColliderType::Capsule:
        {
            float halfSeg = fmaxf(0.0f, collider.Height * 0.5f - collider.Radius);
            Vector3 worldA = Vector3Transform(Vector3Add(collider.Offset, {0, -halfSeg, 0}), worldTransform);
            Vector3 worldB = Vector3Transform(Vector3Add(collider.Offset, {0, halfSeg, 0}), worldTransform);
            float r = collider.Radius;
            box.min = Vector3Subtract(Vector3Min(worldA, worldB), {r, r, r});
            box.max = Vector3Add(Vector3Max(worldA, worldB), {r, r, r});
            return box; // Already in world space
        }
        case ColliderType::Sphere:
        {
            Vector3 worldPos = Vector3Transform(collider.Offset, worldTransform);
            float r = collider.Radius;
            box.min = Vector3Subtract(worldPos, {r, r, r});
            box.max = Vector3Add(worldPos, {r, r, r});
            return box; // Already in world space
        }
    }

    if (!corners.empty())
    {
        box.min = Vector3Transform(corners[0], worldTransform);
        box.max = box.min;
        for (size_t i = 1; i < corners.size(); i++)
        {
            Vector3 worldCorner = Vector3Transform(corners[i], worldTransform);
            box.min = Vector3Min(box.min, worldCorner);
            box.max = Vector3Max(box.max, worldCorner);
        }
    }

    return box;
}

void SceneRenderer::DrawSpawnDebug(entt::registry& registry, const DebugRenderFlags* debugFlags)
{
    auto view = registry.view<TransformComponent, SpawnComponent>();
    for (auto entity : view)
    {
        auto [transform, spawn] = view.get<TransformComponent, SpawnComponent>(entity);
        if (spawn.RenderSpawnZoneInScene)
        {
            Matrix worldTransform = GetWorldTransform(registry, entity);
            Renderer::Get().DrawCubeWires(worldTransform, spawn.ZoneSize, {255, 255, 0, 200});
        }
    }
}

void SceneRenderer::RenderEditorIcons(Scene* scene, const Camera3D& camera)
{
    auto& registry = scene->GetRegistry();
    auto& state = Renderer::Get().GetData();
    auto assetManager = Project::GetActive() ? Project::GetActive()->GetAssetManager() : nullptr;

    if (state.LightIcon.id == 0 && assetManager)
    {
        auto texture = assetManager->Get<TextureAsset>("engine/resources/icons/light_bulb.png");
        if (texture && texture->IsReady())
        {
            state.LightIcon = texture->GetTexture();
        }
    }
    if (state.SpawnIcon.id == 0 && assetManager)
    {
        auto texture = assetManager->Get<TextureAsset>("engine/resources/icons/leaf_icon.png");
        if (texture && texture->IsReady())
        {
            state.SpawnIcon = texture->GetTexture();
        }
    }
    if (state.CameraIcon.id == 0 && assetManager)
    {
        auto texture = assetManager->Get<TextureAsset>("engine/resources/icons/camera_icon.jpg");
        if (texture && texture->IsReady())
        {
            state.CameraIcon = texture->GetTexture();
        }
    }

    rlDisableDepthTest();

    {
        auto view = registry.view<TransformComponent, LightComponent>();
        for (auto entity : view)
        {
            Vector3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, state.LightIcon, worldPos, 1.5f, WHITE);
        }
    }

    {
        auto view = registry.view<TransformComponent, SpawnComponent>();
        for (auto entity : view)
        {
            Vector3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, state.SpawnIcon, worldPos, 1.5f, WHITE);
        }
    }

    {
        auto view = registry.view<TransformComponent, CameraComponent>();
        for (auto entity : view)
        {
            Vector3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, state.CameraIcon, worldPos, 1.5f, WHITE);
        }
    }

    rlEnableDepthTest();
}

void SceneRenderer::RenderSprites(Scene* scene)
{
    CH_CORE_ASSERT(scene, "Scene is null!");
    CH_CORE_ASSERT(Renderer2D::IsInitialized(), "Renderer2D not initialized!");
    auto& registry = scene->GetRegistry();
    auto view = registry.view<TransformComponent, SpriteComponent>();

    std::vector<entt::entity> sortedEntities;
    for (auto entity : view)
    {
        sortedEntities.push_back(entity);
    }

    if (sortedEntities.empty())
    {
        return;
    }

    std::sort(sortedEntities.begin(), sortedEntities.end(), [&](entt::entity a, entt::entity b) {
        return view.get<SpriteComponent>(a).ZOrder < view.get<SpriteComponent>(b).ZOrder;
    });

    Renderer2D::Get().BeginCanvas();
    for (auto entityID : sortedEntities)
    {
        auto& sprite = view.get<SpriteComponent>(entityID);

        if (sprite.TexturePath.empty())
        {
            continue;
        }

        if (!sprite.Texture && Project::GetActive())
        {
            sprite.Texture = Project::GetActive()->GetAssetManager()->Get<TextureAsset>(sprite.TexturePath);
        }

        Vector3 worldPos = GetWorldPosition(registry, entityID);

        Renderer2D::Get().DrawSprite(Vector2{worldPos.x, worldPos.y}, Vector2{1.0f, 1.0f}, 0.0f, sprite.Texture,
                                     sprite.Tint);
    }
    Renderer2D::Get().EndCanvas();
}
} // namespace CHEngine
