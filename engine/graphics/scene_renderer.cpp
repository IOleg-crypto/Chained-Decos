#include "scene_renderer.h"
#include "engine/core/assert.h"
#include "engine/core/profiler.h"
#include "engine/graphics/frustum.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/renderer.h"
#include "engine/graphics/renderer2d.h"
#include "engine/graphics/shader_asset.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/scene/components/light_component.h"
#include "engine/scene/project.h"
#include "imgui.h"
#include "raylib.h"
#include <raymath.h>
#include <rlgl.h>
#include <unordered_map>
#include <unordered_set>

namespace CHEngine
{
SceneRenderer* SceneRenderer::s_Instance = nullptr;

SceneRenderer& SceneRenderer::Get()
{
    CH_CORE_ASSERT(s_Instance, "SceneRenderer is not initialized!");
    return *s_Instance;
}

void SceneRenderer::Init()
{
    if (s_Instance)
    {
        CH_CORE_WARN("SceneRenderer is already initialized!");
        return;
    }
    s_Instance = new SceneRenderer();
    s_Instance->m_Data = std::make_unique<SceneRendererData>();

    // Initialize SSBO for lights
    s_Instance->m_Data->LightSSBO =
        rlLoadShaderBuffer(sizeof(RenderLight) * SceneRendererData::MaxLights, nullptr, RL_DYNAMIC_DRAW);
    s_Instance->m_Data->LightsDirty = true;

    s_Instance->InitializeSkybox();
    CH_CORE_INFO("SceneRenderer Initialized (3D Scene Management).");
}

void SceneRenderer::Shutdown()
{
    if (!s_Instance)
    {
        return;
    }

    CH_CORE_INFO("Shutting down SceneRenderer...");
    if (s_Instance->m_Data->LightSSBO != 0)
    {
        rlUnloadShaderBuffer(s_Instance->m_Data->LightSSBO);
    }

    if (s_Instance->m_Data->SkyboxInitialized)
    {
        UnloadModel(s_Instance->m_Data->SkyboxCube);
    }

    delete s_Instance;
    s_Instance = nullptr;
}
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
    // Transforming origin (0,0,0) by the world transform is a cleaner way to extract position
    // than accessing matrix indices directly.
    return Vector3Transform({0, 0, 0}, GetWorldTransform(registry, entity));
}

void SceneRenderer::Clear(Scene* scene)
{
    if (!scene)
    {
        return;
    }

    auto mode = scene->GetSettings().Mode;
    if (mode == BackgroundMode::Color)
    {
        Renderer::Get().Clear(scene->GetSettings().BackgroundColor);
    }
    else if (mode == BackgroundMode::Texture)
    {
        auto& path = scene->GetSettings().BackgroundTexturePath;
        if (!path.empty())
        {
            // Fallback for now
            Renderer::Get().Clear(scene->GetSettings().BackgroundColor);
        }
    }
    else if (mode == BackgroundMode::Environment3D)
    {
        Renderer::Get().Clear(BLACK);
    }
}

void SceneRenderer::RenderScene(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep,
                                const DebugRenderFlags* debugFlags)
{
    CH_PROFILE_FUNCTION();
    CH_CORE_ASSERT(Renderer::IsInitialized(), "Renderer not initialized!");
    CH_CORE_ASSERT(scene, "Scene is null!");

    // 1. Initial State
    rlEnableDepthTest();

    // 2. Environmental setup
    auto environment = scene->GetSettings().Environment;
    if (!environment && Project::GetActive())
    {
        environment = Project::GetActive()->GetEnvironment();
    }

    // 3. Render Passes
    if (environment)
    {
        ApplyEnvironment(environment->GetSettings());
    }

    Renderer::Get().UpdateTime(Timestep((float)GetTime()));

    // --- Update Profiler Stats ---
    ProfilerStats stats;
    stats.EntityCount = (uint32_t)scene->GetRegistry().storage<entt::entity>().size();
    Profiler::UpdateStats(stats);

    // 2. Scene rendering flow
    m_Data->CurrentCameraPosition = camera.position;
    m_Data->Time = Timestep((float)GetTime());

    Renderer::Get().BeginScene(camera);
    {
        if (environment)
        {
            DrawSkybox(environment->GetSettings().Skybox, camera);
        }

        RenderModels(scene, camera, nearClip, farClip, timestep);

        if (debugFlags)
        {
            RenderDebug(scene, debugFlags);
        }

        RenderSprites(scene);

        RenderEditorIcons(scene, camera);
    }
    Renderer::Get().EndScene();
}

void SceneRenderer::SetLight(int index, const RenderLight& light)
{
    if (index < 0 || index >= SceneRendererData::MaxLights)
    {
        return;
    }
    m_Data->Lights[index] = light;
    m_Data->LightsDirty = true;
}

void SceneRenderer::SetLightCount(int count)
{
    m_Data->LightCount = std::min(count, SceneRendererData::MaxLights);
}

void SceneRenderer::ClearLights()
{
    for (int i = 0; i < SceneRendererData::MaxLights; i++)
    {
        m_Data->Lights[i].enabled = 0;
    }
    m_Data->LightCount = 0;
    m_Data->LightsDirty = true;
}

void SceneRenderer::ApplyEnvironment(const EnvironmentSettings& settings)
{
    m_Data->Lighting = settings.Lighting;
    m_Data->Fog = settings.Fog;
    m_Data->Environment = settings;
    m_Data->LightsDirty = true;
}

void SceneRenderer::InitializeSkybox()
{
    Mesh cube = GenMeshCube(1.0f, 1.0f, 1.0f);
    m_Data->SkyboxCube = LoadModelFromMesh(cube);
    m_Data->SkyboxInitialized = true;
}

void SceneRenderer::DrawSkybox(const SkyboxSettings& skybox, const Camera3D& camera)
{
    if (skybox.TexturePath.empty() || !m_Data->SkyboxInitialized)
    {
        return;
    }

    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    ::DrawModel(m_Data->SkyboxCube, camera.position, 1.0f, WHITE);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

SceneRenderer::InstanceKey::InstanceKey(const std::string& path, const std::vector<MaterialSlot>& mats)
{
    auto hashCombine = [](size_t& seed, size_t hash) { seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2); };

    Hash = std::hash<std::string>{}(path);

    for (const auto& m : mats)
    {
        hashCombine(Hash, std::hash<int>{}((int)m.Target));
        hashCombine(Hash, std::hash<int>{}(m.Index));
        hashCombine(Hash, std::hash<std::string>{}(m.Material.AlbedoPath));
        hashCombine(Hash, std::hash<std::string>{}(m.Material.ShaderPath));
    }
}

void SceneRenderer::RenderModels(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep)
{
    auto& registry = scene->GetRegistry();

    // 1. Frustum Extraction
    Frustum frustum;
    {
        // Use explicit camera matrices for robustness (identical to what BeginMode3D uses)
        Matrix view = GetCameraMatrix(camera);

        // Get aspect ratio from the active window
        float width = (float)GetScreenWidth();
        float height = (float)GetScreenHeight();
        float aspect = (width > 0 && height > 0) ? width / height : 1.0f;

        // Use the passed-in clipping planes from the CameraComponent or Editor camera
        Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, aspect, nearClip, farClip);

        // Standard mathematical order for World -> Clip transformation (for extraction) is Projection * View
        Matrix matVP = MatrixMultiply(view, projection);
        frustum.Extract(matVP);
    }

    // 2. Prepare Lights
    PrepareLights(registry, frustum);

    // 3. Pass A: Collect visible entities
    std::vector<AnimatedEntry> animatedEntries;
    std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash> instanceGroups;
    CollectRenderItems(registry, frustum, animatedEntries, instanceGroups);

    // 4. Pass B: Draw animated individually
    DrawAnimatedEntities(animatedEntries);

    // 5. Pass C: Draw static groups (instanced if >=2, single otherwise)
    DrawStaticEntities(instanceGroups);
}

void SceneRenderer::PrepareLights(entt::registry& registry, const Frustum& frustum)
{
    SceneRenderer::Get().ClearLights();

    int lightCount = 0;
    auto lightView = registry.view<LightComponent>();
    for (auto entity : lightView)
    {
        if (lightCount >= SceneRendererData::MaxLights)
        {
            break;
        }

        auto& light = lightView.get<LightComponent>(entity);
        Matrix worldTransform = GetWorldTransform(registry, entity);
        Vector3 worldPos = {worldTransform.m12, worldTransform.m13, worldTransform.m14};

        // NOTE: Frustum culling for lights disabled temporarily
        // if (!frustum.IsSphereVisible(worldPos, light.Radius))
        //    continue;

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

        SceneRenderer::Get().SetLight(lightCount++, rl);
    }
    SceneRenderer::Get().SetLightCount(lightCount);
}

void SceneRenderer::CollectRenderItems(entt::registry& registry, const Frustum& frustum,
                                       std::vector<AnimatedEntry>& animatedEntries,
                                       std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups)
{
    std::unordered_set<ModelAsset*> updatedAssets;

    auto view = registry.view<TransformComponent, ModelComponent>();
    for (auto entity : view)
    {
        auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);

        if (!model.Asset || model.Asset->GetState() != AssetState::Ready)
        {
            continue;
        }

        // 1. Precise Frustum Culling
        const Matrix& worldTransform = transform.WorldTransform;
        BoundingBox aabb = model.Asset->GetBoundingBox();

        if (!frustum.IsBoxVisible(aabb, worldTransform))
        {
            continue;
        }

        // 2. Optimized Asset Update (Once per unique asset per frame)

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
            DrawModel(model.Asset, worldTransform, model.Materials, 0, 0.0f, -1, 0.0f, 0.0f, shaderOverride,
                      customUniforms);
        }
    }
}

void SceneRenderer::DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries)
{
    for (auto& entry : animatedEntries)
    {
        float targetFPS = 30.0f;
        if (Project::GetActive())
        {
            targetFPS = Project::GetActive()->GetConfig().Animation.TargetFPS;
        }

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

        DrawModel(entry.asset, entry.worldTransform, entry.materials, entry.animation.CurrentAnimationIndex,
                  fractionalFrame, targetAnim, targetFractionalFrame, blendWeight, entry.shaderOverride,
                  entry.customUniforms);
    }
}

void SceneRenderer::DrawStaticEntities(std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups)
{
    for (auto& [key, group] : instanceGroups)
    {
        if (group.transforms.size() == 1)
        {
            DrawModel(group.asset, group.transforms[0], group.materials, 0, 0.0f, -1, 0.0f, 0.0f, nullptr, {});
        }
        else
        {
            DrawModelInstanced(group.asset, group.transforms, group.materials);
        }
    }
}

void SceneRenderer::RenderBVHNode(const BVH* bvh, uint32_t nodeIndex, const Matrix& transform, Color color, int depth)
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
        {
            continue;
        }

        Matrix worldTransform = GetWorldTransform(registry, entity);
        Color color = GREEN;

        switch (collider.Type)
        {
        case ColliderType::Mesh:
            if (collider.BVHRoot)
            {
                RenderBVHNode(collider.BVHRoot.get(), 0, worldTransform, color);
            }
            break;
        case ColliderType::Box: {
            Vector3 center = Vector3Add(collider.Offset, Vector3Scale(collider.Size, 0.5f));
            Matrix colliderTransform = MatrixMultiply(MatrixTranslate(center.x, center.y, center.z), worldTransform);
            Renderer::Get().DrawCubeWires(colliderTransform, collider.Size, color);
            break;
        }
        case ColliderType::Capsule: {
            Matrix colliderTransform = MatrixMultiply(
                MatrixTranslate(collider.Offset.x, collider.Offset.y, collider.Offset.z), worldTransform);
            Renderer::Get().DrawCapsuleWires(colliderTransform, collider.Radius, collider.Height, color);
            break;
        }
        case ColliderType::Sphere: {
            Matrix colliderTransform = MatrixMultiply(
                MatrixTranslate(collider.Offset.x, collider.Offset.y, collider.Offset.z), worldTransform);
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
        {
            continue;
        }

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
        corners = {collider.Offset,
                   Vector3Add(collider.Offset, {collider.Size.x, 0, 0}),
                   Vector3Add(collider.Offset, {0, collider.Size.y, 0}),
                   Vector3Add(collider.Offset, {collider.Size.x, collider.Size.y, 0}),
                   Vector3Add(collider.Offset, {0, 0, collider.Size.z}),
                   Vector3Add(collider.Offset, {collider.Size.x, 0, collider.Size.z}),
                   Vector3Add(collider.Offset, {0, collider.Size.y, collider.Size.z}),
                   Vector3Add(collider.Offset, {collider.Size.x, collider.Size.y, collider.Size.z})};
        break;
    case ColliderType::Capsule: {
        float halfSeg = fmaxf(0.0f, collider.Height * 0.5f - collider.Radius);
        Vector3 worldA = Vector3Transform(Vector3Add(collider.Offset, {0, -halfSeg, 0}), worldTransform);
        Vector3 worldB = Vector3Transform(Vector3Add(collider.Offset, {0, halfSeg, 0}), worldTransform);
        float r = collider.Radius;
        box.min = Vector3Subtract(Vector3Min(worldA, worldB), {r, r, r});
        box.max = Vector3Add(Vector3Max(worldA, worldB), {r, r, r});
        return box; // Already in world space
    }
    case ColliderType::Sphere: {
        Vector3 worldPos = Vector3Transform(collider.Offset, worldTransform);
        float r = collider.Radius;
        box.min = Vector3Subtract(worldPos, {r, r, r});
        box.max = Vector3Add(worldPos, {r, r, r});
        return box; // Already in world space
    }
    }

    if (!corners.empty())
    {
        BoundingBox worldBox;
        worldBox.min = worldBox.max = Vector3Transform(corners[0], worldTransform);
        for (int cornerIndex = 1; cornerIndex < 8; cornerIndex++)
        {
            Vector3 worldCorner = Vector3Transform(corners[cornerIndex], worldTransform);
            worldBox.min = Vector3Min(worldBox.min, worldCorner);
            worldBox.max = Vector3Max(worldBox.max, worldCorner);
        }
        box = worldBox;
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

    if (m_Data->LightIcon.id == 0)
    {
        m_Data->LightIcon = Renderer::Get().GetOrLoadTexture("engine/resources/icons/light_bulb.png");
    }

    if (m_Data->SpawnIcon.id == 0)
    {
        m_Data->SpawnIcon = Renderer::Get().GetOrLoadTexture("engine/resources/icons/leaf_icon.png");
    }

    if (m_Data->CameraIcon.id == 0)
    {
        m_Data->CameraIcon = Renderer::Get().GetOrLoadTexture("engine/resources/icons/camera_icon.jpg");
    }

    rlDisableDepthTest();

    {
        auto view = registry.view<TransformComponent, LightComponent>();
        for (auto entity : view)
        {
            Vector3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, m_Data->LightIcon, worldPos, 1.5f, WHITE);
        }
    }

    {
        auto view = registry.view<TransformComponent, SpawnComponent>();
        for (auto entity : view)
        {
            Vector3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, m_Data->SpawnIcon, worldPos, 1.5f, WHITE);
        }
    }

    {
        auto view = registry.view<TransformComponent, CameraComponent>();
        for (auto entity : view)
        {
            Vector3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, m_Data->CameraIcon, worldPos, 1.5f, WHITE);
        }
    }

    rlEnableDepthTest();
}

void SceneRenderer::RenderSprites(Scene* scene)
{
    CH_CORE_ASSERT(scene, "Scene is null!");
    CH_CORE_ASSERT(Renderer2D::Get().IsInitialized(), "Renderer2D not initialized!");
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

        Texture2D texture = Renderer::Get().GetOrLoadTexture(sprite.TexturePath);
        Vector3 worldPos = GetWorldPosition(registry, entityID);
        Renderer2D::Get().DrawSprite(Vector2{worldPos.x, worldPos.y}, Vector2{1.0f, 1.0f}, texture, sprite.Tint);
    }
    Renderer2D::Get().EndCanvas();
}

void SceneRenderer::DrawModel(const std::shared_ptr<ModelAsset>& modelAsset, const Matrix& transform,
                              const std::vector<MaterialSlot>& materialSlotOverrides, int animationIndex,
                              float frameIndex, int targetAnimationIndex, float targetFrameIndex, float blendWeight,
                              const std::shared_ptr<ShaderAsset>& shaderOverride,
                              const std::vector<ShaderUniform>& shaderUniformOverrides)
{
    if (!modelAsset || modelAsset->GetState() != AssetState::Ready)
    {
        return;
    }

    Model& model = modelAsset->GetModel();

    std::vector<Matrix> boneMatrices = ComputeBoneMatrices(modelAsset, animationIndex, frameIndex, targetAnimationIndex,
                                                           targetFrameIndex, blendWeight);

    for (int i = 0; i < model.meshCount; i++)
    {
        Material material = ResolveMaterialForMesh(i, model, materialSlotOverrides);
        auto activeShaderAsset = shaderOverride ? shaderOverride
                                                : (Renderer::Get().GetShaderLibrary().Exists("Lighting")
                                                       ? Renderer::Get().GetShaderLibrary().Get("Lighting")
                                                       : nullptr);
        Matrix meshTransform = MatrixMultiply(model.transform, transform);

        if (activeShaderAsset)
        {
            ShaderAsset* shader = activeShaderAsset.get();
            if (shader != m_Data->CurrentShader)
            {
                // Shader switch — a non-default override shader; re-upload globals
                rlEnableShader(shader->GetShader().id);
                shader->SetVec3("viewPos", m_Data->CurrentCameraPosition);
                shader->SetFloat("uTime", m_Data->Time);
                shader->SetFloat("uMode", m_Data->DiagnosticMode);
                shader->SetVec3("lightDir", m_Data->Lighting.Direction);
                shader->SetColor("lightColor", m_Data->Lighting.LightColor);
                shader->SetFloat("ambient", m_Data->Lighting.Ambient);
                shader->SetInt("uLightCount", m_Data->LightCount);
                shader->SetFloat("uExposure", m_Data->Lighting.Exposure);
                shader->SetFloat("uGamma", m_Data->Lighting.Gamma);
                ApplyFogUniforms(shader);
                rlBindShaderBuffer(m_Data->LightSSBO, 0);
                m_Data->CurrentShader = shader;
            }

            BindShaderUniforms(shader, boneMatrices, shaderUniformOverrides);
            BindMaterialUniforms(shader, material, i, model, materialSlotOverrides);

            Shader originalShader = material.shader;
            material.shader = shader->GetShader();

            ProfilerStats stats;
            stats.DrawCalls++;
            stats.MeshCount++;
            stats.PolyCount += model.meshes[i].triangleCount;
            Profiler::UpdateStats(stats);

            // DrawMesh is likely static in Renderer or we use raylib's DrawMesh?
            // Usually DrawMesh is a low-level utility.
            ::DrawMesh(model.meshes[i], material, meshTransform);
            material.shader = originalShader;
        }
        else
        {
            ::DrawMesh(model.meshes[i], material, meshTransform);
        }
    }
}

void SceneRenderer::DrawModelInstanced(const std::shared_ptr<ModelAsset>& modelAsset,
                                       const std::vector<Matrix>& transforms,
                                       const std::vector<MaterialSlot>& materialSlotOverrides)
{
    if (!modelAsset || modelAsset->GetState() != AssetState::Ready || transforms.empty())
    {
        return;
    }

    Model& model = modelAsset->GetModel();
    for (int i = 0; i < model.meshCount; i++)
    {
        Material material = ResolveMaterialForMesh(i, model, materialSlotOverrides);
        auto lightingShader = Renderer::Get().GetShaderLibrary().Exists("Lighting")
                                  ? Renderer::Get().GetShaderLibrary().Get("Lighting")
                                  : nullptr;

        if (lightingShader)
        {
            ShaderAsset* shader = lightingShader.get();
            if (shader != m_Data->CurrentShader)
            {
                rlEnableShader(shader->GetShader().id);
                shader->SetVec3("viewPos", m_Data->CurrentCameraPosition);
                shader->SetFloat("uTime", m_Data->Time);
                shader->SetFloat("uMode", m_Data->DiagnosticMode);
                shader->SetVec3("lightDir", m_Data->Lighting.Direction);
                shader->SetColor("lightColor", m_Data->Lighting.LightColor);
                shader->SetFloat("ambient", m_Data->Lighting.Ambient);
                shader->SetInt("uLightCount", m_Data->LightCount);
                shader->SetInt("uLightCount", m_Data->LightCount);
                shader->SetFloat("uExposure", m_Data->Lighting.Exposure);
                shader->SetFloat("uGamma", m_Data->Lighting.Gamma);
                ApplyFogUniforms(shader);
                rlBindShaderBuffer(m_Data->LightSSBO, 0);
                m_Data->CurrentShader = shader;
            }

            BindMaterialUniforms(shader, material, i, model, materialSlotOverrides);

            Shader originalShader = material.shader;
            material.shader = shader->GetShader();

            ProfilerStats stats;
            stats.DrawCalls++;
            stats.MeshCount++;
            stats.PolyCount += model.meshes[i].triangleCount * (int)transforms.size();
            Profiler::UpdateStats(stats);

            ::DrawMeshInstanced(model.meshes[i], material, transforms.data(), (int)transforms.size());
            material.shader = originalShader;
        }
        else
        {
            ::DrawMeshInstanced(model.meshes[i], material, transforms.data(), (int)transforms.size());
        }
    }
}

void SceneRenderer::ApplyFogUniforms(ShaderAsset* shader)
{
    if (!shader)
    {
        return;
    }
    auto& fog = m_Data->Fog;
    shader->SetInt("fogEnabled", fog.Enabled ? 1 : 0);
    if (fog.Enabled)
    {
        shader->SetColor("fogColor", fog.FogColor);
        shader->SetFloat("fogDensity", fog.Density);
        shader->SetFloat("fogStart", fog.Start);
        shader->SetFloat("fogEnd", fog.End);
    }
}

std::vector<Matrix> SceneRenderer::ComputeBoneMatrices(const std::shared_ptr<ModelAsset>& modelAsset,
                                                       int animationIndex, float frameIndex, int targetAnimationIndex,
                                                       float targetFrameIndex, float blendWeight)
{
    if (!modelAsset)
    {
        return {};
    }

    Model& model = modelAsset->GetModel();
    if (model.boneCount <= 0)
    {
        return {};
    }

    const auto& offsetMatrices = modelAsset->GetOffsetMatrices();
    if (offsetMatrices.empty())
    {
        CH_CORE_WARN("ModelAsset '{}' has bones but no offset matrices loaded.", modelAsset->GetPath());
        return {};
    }

    const int boneCount = model.boneCount;

    // Reuse pre-allocated scratch buffers — avoids heap allocations each frame
    auto& boneMatrices = m_Data->ScratchBoneMatrices;
    auto& globalPose = m_Data->ScratchGlobalPose;
    auto& localPoseA = m_Data->ScratchLocalPoseA;
    boneMatrices.resize(boneCount);
    globalPose.resize(boneCount);
    localPoseA.resize(boneCount);

    auto CalculateLocalPose = [&](int animIdx, float fIdx, std::vector<Transform>& outLocalPose) {
        const auto& animations = modelAsset->GetRawAnimations();
        if (animIdx >= 0 && animIdx < (int)animations.size())
        {
            const auto& anim = animations[animIdx];
            int currentFrame = (int)fIdx % anim.frameCount;
            int nextFrame = (currentFrame + 1) % anim.frameCount;
            float interp = fIdx - (float)((int)fIdx);

            for (int i = 0; i < anim.boneCount; i++)
            {
                Transform t = anim.framePoses[currentFrame * anim.boneCount + i];
                Transform tNext = anim.framePoses[nextFrame * anim.boneCount + i];

                outLocalPose[i].translation = Vector3Lerp(t.translation, tNext.translation, interp);
                outLocalPose[i].rotation = QuaternionSlerp(t.rotation, tNext.rotation, interp);
                outLocalPose[i].scale = Vector3Lerp(t.scale, tNext.scale, interp);
            }
            return true;
        }
        return false;
    };

    bool hasA = CalculateLocalPose(animationIndex, frameIndex, localPoseA);
    if (!hasA)
    {
        for (int i = 0; i < boneCount; i++)
        {
            localPoseA[i] = model.bindPose[i];
        }
    }

    if (targetAnimationIndex >= 0 && blendWeight > 0.0f)
    {
        auto& localPoseB = m_Data->ScratchLocalPoseB;
        localPoseB.resize(boneCount);
        if (CalculateLocalPose(targetAnimationIndex, targetFrameIndex, localPoseB))
        {
            for (int i = 0; i < boneCount; i++)
            {
                localPoseA[i].translation =
                    Vector3Lerp(localPoseA[i].translation, localPoseB[i].translation, blendWeight);
                localPoseA[i].rotation = QuaternionSlerp(localPoseA[i].rotation, localPoseB[i].rotation, blendWeight);
                localPoseA[i].scale = Vector3Lerp(localPoseA[i].scale, localPoseB[i].scale, blendWeight);
            }
        }
    }

    // Convert local transforms to global matrices
    for (int i = 0; i < boneCount; i++)
    {
        Matrix localMat = MatrixMultiply(
            QuaternionToMatrix(localPoseA[i].rotation),
            MatrixTranslate(localPoseA[i].translation.x, localPoseA[i].translation.y, localPoseA[i].translation.z));
        localMat =
            MatrixMultiply(MatrixScale(localPoseA[i].scale.x, localPoseA[i].scale.y, localPoseA[i].scale.z), localMat);

        int parent = model.bones[i].parent;
        globalPose[i] = (parent == -1) ? localMat : MatrixMultiply(globalPose[parent], localMat);
    }

    // Compute final skinning matrices
    for (int i = 0; i < boneCount; i++)
    {
        boneMatrices[i] =
            (i < (int)offsetMatrices.size()) ? MatrixMultiply(offsetMatrices[i], globalPose[i]) : globalPose[i];
    }

    return boneMatrices;
}

Material SceneRenderer::ResolveMaterialForMesh(int meshIndex, const Model& model,
                                               const std::vector<MaterialSlot>& materialSlotOverrides)
{
    Material material = model.materials[model.meshMaterial[meshIndex]];

    for (const auto& slot : materialSlotOverrides)
    {
        bool match = false;
        if (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == meshIndex)
        {
            match = true;
        }
        else if (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.meshMaterial[meshIndex])
        {
            match = true;
        }

        if (match)
        {
            material.maps[MATERIAL_MAP_ALBEDO].color = slot.Material.AlbedoColor;
            if (slot.Material.OverrideAlbedo && !slot.Material.AlbedoPath.empty())
            {
                material.maps[MATERIAL_MAP_ALBEDO].texture = Renderer::Get().GetOrLoadTexture(slot.Material.AlbedoPath);
            }
            if (slot.Material.OverrideNormal && !slot.Material.NormalMapPath.empty())
            {
                material.maps[MATERIAL_MAP_NORMAL].texture =
                    Renderer::Get().GetOrLoadTexture(slot.Material.NormalMapPath);
            }
            if (slot.Material.OverrideMetallicRoughness && !slot.Material.MetallicRoughnessPath.empty())
            {
                material.maps[MATERIAL_MAP_METALNESS].texture =
                    Renderer::Get().GetOrLoadTexture(slot.Material.MetallicRoughnessPath);
                material.maps[MATERIAL_MAP_ROUGHNESS].texture =
                    Renderer::Get().GetOrLoadTexture(slot.Material.MetallicRoughnessPath);
            }
            if (slot.Material.OverrideOcclusion && !slot.Material.OcclusionMapPath.empty())
            {
                material.maps[MATERIAL_MAP_OCCLUSION].texture =
                    Renderer::Get().GetOrLoadTexture(slot.Material.OcclusionMapPath);
            }
            if (slot.Material.OverrideEmissive && !slot.Material.EmissivePath.empty())
            {
                material.maps[MATERIAL_MAP_EMISSION].texture =
                    Renderer::Get().GetOrLoadTexture(slot.Material.EmissivePath);
            }
            break;
        }
    }
    return material;
}

void SceneRenderer::BindShaderUniforms(ShaderAsset* activeShader, const std::vector<Matrix>& boneMatrices,
                                       const std::vector<ShaderUniform>& shaderUniformOverrides)
{
    if (!activeShader)
    {
        return;
    }

    if (!boneMatrices.empty())
    {
        int count = (int)boneMatrices.size();
        if (count > 128)
        {
            count = 128; // Shader limit
        }
        activeShader->SetMatrices("boneMatrices", boneMatrices.data(), count);
    }
    else
    {
        static Matrix identityMatrices[4] = {MatrixIdentity(), MatrixIdentity(), MatrixIdentity(), MatrixIdentity()};
        activeShader->SetMatrices("boneMatrices", identityMatrices, 4);
    }

    for (const auto& u : shaderUniformOverrides)
    {
        if (u.Type == 0)
        {
            activeShader->SetFloat(u.Name, u.Value[0]);
        }
        else if (u.Type == 1)
        {
            activeShader->SetVec2(u.Name, {u.Value[0], u.Value[1]});
        }
        else if (u.Type == 2)
        {
            activeShader->SetVec3(u.Name, {u.Value[0], u.Value[1], u.Value[2]});
        }
        else if (u.Type == 3)
        {
            activeShader->SetVec4(u.Name, {u.Value[0], u.Value[1], u.Value[2], u.Value[3]});
        }
        else if (u.Type == 4)
        {
            activeShader->SetColor(u.Name, Color{(unsigned char)(u.Value[0] * 255), (unsigned char)(u.Value[1] * 255),
                                                 (unsigned char)(u.Value[2] * 255), (unsigned char)(u.Value[3] * 255)});
        }
    }
}

void SceneRenderer::BindMaterialUniforms(ShaderAsset* activeShader, const Material& material, int meshIndex,
                                         const Model& model, const std::vector<MaterialSlot>& materialSlotOverrides)
{
    if (!activeShader)
    {
        return;
    }

    activeShader->SetInt("useTexture", material.maps[MATERIAL_MAP_ALBEDO].texture.id > 0 ? 1 : 0);
    activeShader->SetColor("colDiffuse", material.maps[MATERIAL_MAP_ALBEDO].color);

    int useNormalMap = material.maps[MATERIAL_MAP_NORMAL].texture.id > 0 ? 1 : 0;
    int useMetallicMap = material.maps[MATERIAL_MAP_METALNESS].texture.id > 0 ? 1 : 0;
    int useRoughnessMap = material.maps[MATERIAL_MAP_ROUGHNESS].texture.id > 0 ? 1 : 0;
    int useOcclusionMap = material.maps[MATERIAL_MAP_OCCLUSION].texture.id > 0 ? 1 : 0;
    int useEmissiveTexture = material.maps[MATERIAL_MAP_EMISSION].texture.id > 0 ? 1 : 0;

    activeShader->SetInt("useNormalMap", useNormalMap);
    activeShader->SetInt("useMetallicMap", useMetallicMap);
    activeShader->SetInt("useRoughnessMap", useRoughnessMap);
    activeShader->SetInt("useOcclusionMap", useOcclusionMap);
    activeShader->SetInt("useEmissiveTexture", useEmissiveTexture);

    float metalness = material.maps[MATERIAL_MAP_METALNESS].value;
    float roughness = material.maps[MATERIAL_MAP_ROUGHNESS].value;

    Color colEmissive = material.maps[MATERIAL_MAP_EMISSION].color;
    float emissiveIntensity = 0.0f;

    for (const auto& slot : materialSlotOverrides)
    {
        bool match = false;
        if (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == meshIndex)
        {
            match = true;
        }
        else if (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.meshMaterial[meshIndex])
        {
            match = true;
        }
        if (match)
        {
            emissiveIntensity = slot.Material.EmissiveIntensity;
            if (slot.Material.OverrideEmissive)
            {
                colEmissive = slot.Material.EmissiveColor;
            }
            metalness = slot.Material.Metalness;
            roughness = slot.Material.Roughness;
            break;
        }
    }

    activeShader->SetFloat("metalness", metalness);
    activeShader->SetFloat("roughness", roughness);

    if (emissiveIntensity == 0.0f && (colEmissive.r > 0 || colEmissive.g > 0 || colEmissive.b > 0))
    {
        emissiveIntensity = 1.0f;
    }

    activeShader->SetColor("colEmissive", colEmissive);
    activeShader->SetFloat("emissiveIntensity", emissiveIntensity);

    float shininess = (1.0f - roughness) * 128.0f;
    if (shininess < 1.0f)
    {
        shininess = 1.0f;
    }
    activeShader->SetFloat("shininess", shininess);
}
} // namespace CHEngine
