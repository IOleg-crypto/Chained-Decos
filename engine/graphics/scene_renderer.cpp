#include "scene_renderer.h"
#include "engine/core/assert.h"
#include "engine/core/profiler.h"
#include "engine/graphics/frustum.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/mesh_importer.h"
#include "engine/graphics/renderer.h"
#include "engine/graphics/renderer2d.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/shader_asset.h"
#include "engine/physics/physics.h"
#include "engine/scene/components/light_component.h"
#include "imgui.h"
#include "raylib.h"
#include <raymath.h>
#include <rlgl.h>
#include <unordered_map>
#include <unordered_set>


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
    // Transforming origin (0,0,0) by the world transform is a cleaner way to extract position
    // than accessing matrix indices directly.
    return Vector3Transform({0, 0, 0}, GetWorldTransform(registry, entity));
}

void SceneRenderer::RenderScene(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep,
                                const SceneRenderOptions& options)
{
    CH_PROFILE_FUNCTION();
    CH_CORE_ASSERT(Renderer::IsInitialized(), "Renderer not initialized!");
    CH_CORE_ASSERT(scene, "Scene is null!");

    // 1. Initial State
    rlEnableDepthTest();

    // 2. Environmental setup
    auto environment = scene->GetSettings().Environment;
    if (!environment)
    {
        environment = options.EnvironmentOverride;
    }

    // 3. Render Passes
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
        CH_CORE_WARN_ONCE("SceneRenderer: No environment asset for scene!");
    }

    Renderer::Get().UpdateTime(Timestep((float)GetTime()));

    // --- Update Profiler Stats ---
    m_CurrentStats = {};
    m_CurrentStats.EntityCount = (uint32_t)scene->GetRegistry().storage<entt::entity>().size();

    // 2. Scene rendering flow
    Renderer::Get().BeginScene(camera);
    {
        if (environment)
        {
            if (environment->GetSettings().Skybox.TexturePath.empty())
            {
                CH_CORE_WARN_ONCE("SceneRenderer: Environment exists but Skybox.TexturePath is empty!");
            }
            Renderer::Get().DrawSkybox(environment->GetSettings().Skybox, camera);
        }
        else
        {
            CH_CORE_WARN_ONCE("SceneRenderer: No environment asset for scene!");
        }

        RenderModels(scene, camera, nearClip, farClip, timestep, options);

        RenderDebug(scene, options);

        RenderSprites(scene);

        RenderEditorIcons(scene, camera);
    }
    Renderer::Get().EndScene();

    // 4. Report final stats
    Profiler::UpdateStats(m_CurrentStats);
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

void SceneRenderer::RenderModels(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep,
                                 const SceneRenderOptions& options)
{
    auto& registry = scene->GetRegistry();

    // 1. Frustum Extraction
    Frustum frustum;
    {
        // Use explicit camera matrices for robustness (identical to what BeginMode3D uses)
        Matrix view = GetCameraMatrix(camera);

        // Get aspect ratio from the active render target/window
        float width = (float)GetRenderWidth();
        float height = (float)GetRenderHeight();
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
    DrawAnimatedEntities(animatedEntries, options);

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
        if (lightCount >= LightingData::MaxLights)
        {
            break;
        }

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
                                       std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups)
{
    std::unordered_set<ModelAsset*> updatedAssets;

    // 3. Collect Models
    auto view = registry.view<TransformComponent, ModelComponent>();
    for (auto entity : view)
    {
        auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);

        if (model.ModelPath.empty())
            continue;

        auto modelAsset = AssetManager::Get().Get<ModelAsset>(model.ModelPath);
        if (!modelAsset || modelAsset->GetState() != AssetState::Ready)
        {
            continue;
        }

        // 1. Precise Frustum Culling
        const Matrix& worldTransform = transform.WorldTransform;
        BoundingBox aabb = modelAsset->GetBoundingBox();

        if (!frustum.IsBoxVisible(aabb, worldTransform))
        {
            continue;
        }

        // 2. Optimized Asset Update (Once per unique asset per frame)
        if (updatedAssets.find(modelAsset.get()) == updatedAssets.end())
        {
            modelAsset->OnUpdate();
            updatedAssets.insert(modelAsset.get());
        }

        std::shared_ptr<ShaderAsset> shaderOverride;
        std::vector<ShaderUniform> customUniforms;
        bool hasShaderOverride = false;
        if (registry.all_of<ShaderComponent>(entity))
        {
            auto& shaderComp = registry.get<ShaderComponent>(entity);
            if (shaderComp.Enabled && !shaderComp.ShaderPath.empty())
            {
                shaderOverride = AssetManager::Get().Get<ShaderAsset>(shaderComp.ShaderPath);
                customUniforms = shaderComp.Uniforms;
                hasShaderOverride = true;
            }
        }

        const bool isAnimated = registry.all_of<AnimationComponent>(entity);

        if (isAnimated)
        {
            AnimatedEntry entry;
            entry.asset = modelAsset;
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
                group.asset = modelAsset;
                group.materials = model.Materials;
            }
            group.transforms.push_back(worldTransform);
        }
        else
        {
            DrawModel(modelAsset, worldTransform, model.Materials, {}, shaderOverride, customUniforms);
        }
    }

    // 4. Collect Primitives
    auto primitiveView = registry.view<TransformComponent, PrimitiveComponent>();
    for (auto entity : primitiveView)
    {
        auto [transform, primitive] = primitiveView.get<TransformComponent, PrimitiveComponent>(entity);

        if (primitive.Type == PrimitiveType::None)
            continue;

        // Lazy load/cache primitive asset or regenerate if dirty
        if ((!primitive.Asset || primitive.Dirty))
        {
            const char* primitivePaths[] = {
                "", ":cube:", ":sphere:", ":plane:", ":cylinder:", ":cone:", ":torus:", ":knot:", ":hemisphere:"
            };
            int typeIdx = (int)primitive.Type;
            if (typeIdx > 0 && typeIdx < (int)std::size(primitivePaths))
            {
                ProceduralParameters params;
                params.Radius = primitive.Radius;
                params.InnerRadius = primitive.InnerRadius;
                params.Height = primitive.Height;
                params.Slices = primitive.Slices;
                params.Stacks = primitive.Stacks;
                params.Dimensions = primitive.Dimensions;

                // Create a temporary model from updated parameters
                Model model = MeshImporter::GenerateProceduralModel(primitivePaths[typeIdx], params);
                if (model.meshCount > 0)
                {
                    if (!primitive.Asset)
                    {
                        primitive.Asset = std::make_shared<ModelAsset>();
                        primitive.Asset->SetPath(primitivePaths[typeIdx]);
                    }
                    else
                    {
                        // Unload existing mesh data if regenerating
                        UnloadModel(primitive.Asset->GetModel());
                    }
                    
                    primitive.Asset->GetModel() = model;
                    primitive.Asset->SetState(AssetState::Ready);
                }
                primitive.Dirty = false;
            }
        }

        if (!primitive.Asset || primitive.Asset->GetState() != AssetState::Ready)
            continue;

        const Matrix& worldTransform = transform.WorldTransform;
        BoundingBox aabb = primitive.Asset->GetBoundingBox();

        if (!frustum.IsBoxVisible(aabb, worldTransform))
            continue;

        // 2. Optimized Asset Update
        if (updatedAssets.find(primitive.Asset.get()) == updatedAssets.end())
        {
            primitive.Asset->OnUpdate();
            updatedAssets.insert(primitive.Asset.get());
        }

        // Handle shader override for primitives too
        std::shared_ptr<ShaderAsset> shaderOverride;
        std::vector<ShaderUniform> customUniforms;
        bool hasShaderOverride = false;
        if (registry.all_of<ShaderComponent>(entity))
        {
            auto& shaderComp = registry.get<ShaderComponent>(entity);
            if (shaderComp.Enabled && !shaderComp.ShaderPath.empty())
            {
                shaderOverride = AssetManager::Get().Get<ShaderAsset>(shaderComp.ShaderPath);
                customUniforms = shaderComp.Uniforms;
                hasShaderOverride = true;
            }
        }

        // Primitives are never animated (for now)
        if (!hasShaderOverride)
        {
            std::string pathKey = "primitive_" + std::to_string((int)primitive.Type);
            InstanceKey key{pathKey, {}}; // No material slots for primitives yet
            auto& group = instanceGroups[key];
            if (group.transforms.empty())
            {
                group.asset = primitive.Asset;
                group.materials = {};
            }
            group.transforms.push_back(worldTransform);
        }
        else
        {
            DrawModel(primitive.Asset, worldTransform, {}, {}, shaderOverride, customUniforms);
        }
    }
}

void SceneRenderer::DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries, const SceneRenderOptions& options)
{
    for (auto& entry : animatedEntries)
    {
        float targetFPS = options.TargetFPS;

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

        std::vector<Matrix> boneMatrices = entry.asset->ComputeAnimationPose(
            entry.animation.CurrentAnimationIndex, fractionalFrame, targetAnim, targetFractionalFrame, blendWeight);

        DrawModel(entry.asset, entry.worldTransform, entry.materials, boneMatrices, entry.shaderOverride, entry.customUniforms);
    }
}

void SceneRenderer::DrawStaticEntities(std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups)
{
    for (auto& [key, group] : instanceGroups)
    {
        for (const auto& transform : group.transforms)
        {
            DrawModel(group.asset, transform, group.materials);
        }
    }
}

void SceneRenderer::RenderDebug(Scene* scene, const SceneRenderOptions& options)
{
    if (!options.ShowDebugColliders && !options.ShowDebugCollisionModelBox && !options.ShowDebugSpawnZones && !options.DrawGrid)
    {
        return;
    }
    auto& registry = scene->GetRegistry();

    rlDisableDepthTest();


    if (options.DrawGrid && scene->GetSettings().Mode == BackgroundMode::Environment3D)
    {
        const auto& grid = scene->GetSettings().Grid;
        Renderer::Get().DrawGrid(grid.Slices, grid.Spacing);
    }

    if (options.ShowDebugColliders)
    {
        DrawColliderDebug(registry, options);
    }
    if (options.ShowDebugCollisionModelBox)
    {
        DrawCollisionModelBoxDebug(registry, options);
    }
    if (options.ShowDebugSpawnZones)
    {
        DrawSpawnDebug(registry, options);
    }

    rlEnableDepthTest();
}

void SceneRenderer::DrawColliderDebug(entt::registry& registry, const SceneRenderOptions& options)
{
    auto view = registry.view<TransformComponent, ColliderComponent>();
    for (auto entity : view)
    {
        auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);
        if (!collider.Enabled) continue;

        Matrix worldTransform = GetWorldTransform(registry, entity);
        Color debugColor = collider.IsColliding ? RED : LIME;

        if (collider.Type == ColliderType::Box)
        {
            Renderer::Get().DrawCubeWires(MatrixMultiply(MatrixTranslate(collider.Offset.x, collider.Offset.y, collider.Offset.z), worldTransform), collider.Size, debugColor);
        }
        else if (collider.Type == ColliderType::Sphere)
        {
            Renderer::Get().DrawSphereWires(MatrixMultiply(MatrixTranslate(collider.Offset.x, collider.Offset.y, collider.Offset.z), worldTransform), collider.Radius, debugColor);
        }
        else if (collider.Type == ColliderType::Capsule)
        {
            Renderer::Get().DrawCapsuleWires(MatrixMultiply(MatrixTranslate(collider.Offset.x, collider.Offset.y, collider.Offset.z), worldTransform), collider.Radius, collider.Height, debugColor);
        }
        else if (collider.Type == ColliderType::Mesh)
        {
            if (!collider.ModelPath.empty())
            {
                auto model = AssetManager::Get().Get<ModelAsset>(collider.ModelPath);
                if (model && model->GetState() == AssetState::Ready)
                {
                    const auto& rayModel = model->GetModel();
                    const auto& nodeTransforms = model->GetGlobalNodeTransforms();
                    const auto& meshToNode = model->GetMeshToNode();

                    for (int i = 0; i < rayModel.meshCount; i++)
                    {
                        Matrix nodeMat = MatrixIdentity();
                        if (i < (int)meshToNode.size() && meshToNode[i] >= 0)
                            nodeMat = nodeTransforms[meshToNode[i]];
                        
                        Matrix finalTransform = MatrixMultiply(nodeMat, MatrixMultiply(rayModel.transform, worldTransform));
                        Renderer::Get().DrawMeshWire(rayModel.meshes[i], debugColor, finalTransform);
                    }
                }
            }
        }
        
        m_CurrentStats.ColliderCount++;
    }
}

void SceneRenderer::DrawCollisionModelBoxDebug(entt::registry& registry, const SceneRenderOptions& options)
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
    BoundingBox box = {};
    Vector3 extents = Vector3Scale(collider.Size, 0.5f);
    Vector3 minLocal = Vector3Subtract(collider.Offset, extents);
    Vector3 maxLocal = Vector3Add(collider.Offset, extents);

    Vector3 corners[8] = {
        {minLocal.x, minLocal.y, minLocal.z}, {maxLocal.x, minLocal.y, minLocal.z},
        {minLocal.x, maxLocal.y, minLocal.z}, {maxLocal.x, maxLocal.y, minLocal.z},
        {minLocal.x, minLocal.y, maxLocal.z}, {maxLocal.x, minLocal.y, maxLocal.z},
        {minLocal.x, maxLocal.y, maxLocal.z}, {maxLocal.x, maxLocal.y, maxLocal.z}
    };

    Vector3 minWorld = {FLT_MAX, FLT_MAX, FLT_MAX};
    Vector3 maxWorld = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

    for (int i = 0; i < 8; i++)
    {
        Vector3 worldPt = Vector3Transform(corners[i], worldTransform);
        minWorld.x = std::min(minWorld.x, worldPt.x);
        minWorld.y = std::min(minWorld.y, worldPt.y);
        minWorld.z = std::min(minWorld.z, worldPt.z);
        maxWorld.x = std::max(maxWorld.x, worldPt.x);
        maxWorld.y = std::max(maxWorld.y, worldPt.y);
        maxWorld.z = std::max(maxWorld.z, worldPt.z);
    }
    box.min = minWorld;
    box.max = maxWorld;

    return box;
}

void SceneRenderer::DrawSpawnDebug(entt::registry& registry, const SceneRenderOptions& options)
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
    auto& assetManager = AssetManager::Get();

    if (m_EditorResources.LightIcon.id == 0)
    {
        auto texture = assetManager.Get<TextureAsset>("resources/icons/light_bulb.png");
        if (texture && texture->IsReady()) m_EditorResources.LightIcon = texture->GetTexture();
    }
    if (m_EditorResources.SpawnIcon.id == 0)
    {
        auto texture = assetManager.Get<TextureAsset>("resources/icons/leaf_icon.png");
        if (texture && texture->IsReady()) m_EditorResources.SpawnIcon = texture->GetTexture();
    }
    if (m_EditorResources.CameraIcon.id == 0)
    {
        auto texture = assetManager.Get<TextureAsset>("resources/icons/camera_icon.png");
        if (texture && texture->IsReady()) m_EditorResources.CameraIcon = texture->GetTexture();
    }

    rlDisableDepthTest();
    {
        auto view = registry.view<TransformComponent, LightComponent>();
        for (auto entity : view)
        {
            Vector3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, m_EditorResources.LightIcon, worldPos, 1.5f, WHITE);
        }
    }
    {
        auto view = registry.view<TransformComponent, SpawnComponent>();
        for (auto entity : view)
        {
            Vector3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, m_EditorResources.SpawnIcon, worldPos, 1.5f, WHITE);
        }
    }
    {
        auto view = registry.view<TransformComponent, CameraComponent>();
        for (auto entity : view)
        {
            Vector3 worldPos = GetWorldPosition(registry, entity);
            Renderer::Get().DrawBillboard(camera, m_EditorResources.CameraIcon, worldPos, 1.5f, WHITE);
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

        if (!sprite.Texture)
        {
            sprite.Texture = AssetManager::Get().Get<TextureAsset>(sprite.TexturePath);
        }

        Vector3 worldPos = GetWorldPosition(registry, entityID);

        Renderer2D::Get().DrawSprite(Vector2{worldPos.x, worldPos.y}, Vector2{1.0f, 1.0f}, 0.0f, sprite.Texture,
                                     sprite.Tint);
    }
    Renderer2D::Get().EndCanvas();
}
void SceneRenderer::DrawModel(const std::shared_ptr<ModelAsset>& modelAsset, const Matrix& transform,
                               const std::vector<MaterialSlot>& materialSlotOverrides,
                               const std::vector<Matrix>& boneMatrices,
                               const std::shared_ptr<ShaderAsset>& shaderOverride,
                               const std::vector<ShaderUniform>& shaderUniformOverrides)
{
    if (!modelAsset || modelAsset->GetState() != AssetState::Ready) return;

    Model& model = modelAsset->GetModel();
    const auto& globalNodeTransforms = modelAsset->GetGlobalNodeTransforms();
    const auto& meshToNode = modelAsset->GetMeshToNode();

    auto& renderer = Renderer::Get();
    auto activeShader = shaderOverride ? shaderOverride : (renderer.GetShaderLibrary().Exists("Lighting") ? renderer.GetShaderLibrary().Get("Lighting") : nullptr);

    for (int i = 0; i < model.meshCount; i++)
    {
        m_CurrentStats.DrawCalls++;
        m_CurrentStats.MeshCount++;
        m_CurrentStats.PolyCount += model.meshes[i].triangleCount;

        Material material = ResolveMaterialForMesh(i, model, materialSlotOverrides);
        
        Matrix nodeTransform = MatrixIdentity();
        if (i < (int)meshToNode.size())
        {
            int nodeIdx = meshToNode[i];
            if (nodeIdx >= 0 && nodeIdx < (int)globalNodeTransforms.size()) 
                nodeTransform = globalNodeTransforms[nodeIdx];
        }

        Matrix meshWorldTransform = MatrixMultiply(nodeTransform, MatrixMultiply(model.transform, transform));

        if (activeShader)
        {
            BindShaderUniforms(activeShader.get(), boneMatrices, shaderUniformOverrides);
            BindMaterialUniforms(activeShader.get(), material, i, model, materialSlotOverrides);

            Shader originalShader = material.shader;
            material.shader = activeShader->GetShader();
            renderer.DrawMesh(model.meshes[i], material, meshWorldTransform);
            material.shader = originalShader;
        }
        else
        {
            renderer.DrawMesh(model.meshes[i], material, meshWorldTransform);
        }
    }
}

Material SceneRenderer::ResolveMaterialForMesh(int meshIndex, const Model& model, const std::vector<MaterialSlot>& materialSlotOverrides)
{
    Material material = model.materials[model.meshMaterial[meshIndex]];
    for (const auto& slot : materialSlotOverrides)
    {
        bool match = (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == meshIndex) ||
                     (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.meshMaterial[meshIndex]);
        if (match)
        {
            material.maps[MATERIAL_MAP_ALBEDO].color = slot.Material.AlbedoColor;
            if (slot.Material.OverrideAlbedo && !slot.Material.AlbedoPath.empty())
            {
                auto tex = AssetManager::Get().Get<TextureAsset>(slot.Material.AlbedoPath);
                if (tex && tex->IsReady()) material.maps[MATERIAL_MAP_ALBEDO].texture = tex->GetTexture();
            }
            break;
        }
    }
    return material;
}

void SceneRenderer::BindShaderUniforms(ShaderAsset* activeShader, const std::vector<Matrix>& boneMatrices, const std::vector<ShaderUniform>& shaderUniformOverrides)
{
    if (!activeShader) return;
    if (!boneMatrices.empty())
    {
        int count = std::min((int)boneMatrices.size(), 128);
        activeShader->SetMatrices("boneMatrices", boneMatrices.data(), count);
    }
    else
    {
        static Matrix identities[4] = { MatrixIdentity(), MatrixIdentity(), MatrixIdentity(), MatrixIdentity() };
        activeShader->SetMatrices("boneMatrices", identities, 4);
    }

    for (const auto& u : shaderUniformOverrides)
    {
        switch (u.Type)
        {
            case 0: activeShader->SetFloat(u.Name, u.Value[0]); break;
            case 1: activeShader->SetVec2(u.Name, {u.Value[0], u.Value[1]}); break;
            case 2: activeShader->SetVec3(u.Name, {u.Value[0], u.Value[1], u.Value[2]}); break;
            case 3: activeShader->SetVec4(u.Name, {u.Value[0], u.Value[1], u.Value[2], u.Value[3]}); break;
            case 4: activeShader->SetColor(u.Name, Color{(unsigned char)(u.Value[0]*255), (unsigned char)(u.Value[1]*255), (unsigned char)(u.Value[2]*255), (unsigned char)(u.Value[3]*255)}); break;
        }
    }
}

void SceneRenderer::BindMaterialUniforms(ShaderAsset* activeShader, const Material& material, int meshIndex, const Model& model, const std::vector<MaterialSlot>& materialSlotOverrides)
{
    if (!activeShader) return;
    activeShader->SetInt("useTexture", material.maps[MATERIAL_MAP_ALBEDO].texture.id > 0 ? 1 : 0);
    activeShader->SetColor("colDiffuse", material.maps[MATERIAL_MAP_ALBEDO].color);
    activeShader->SetInt("useNormalMap", material.maps[MATERIAL_MAP_NORMAL].texture.id > 0 ? 1 : 0);
    activeShader->SetInt("useMetallicMap", material.maps[MATERIAL_MAP_METALNESS].texture.id > 0 ? 1 : 0);
    activeShader->SetInt("useRoughnessMap", material.maps[MATERIAL_MAP_ROUGHNESS].texture.id > 0 ? 1 : 0);
    activeShader->SetInt("useOcclusionMap", material.maps[MATERIAL_MAP_OCCLUSION].texture.id > 0 ? 1 : 0);
    activeShader->SetInt("useEmissiveTexture", material.maps[MATERIAL_MAP_EMISSION].texture.id > 0 ? 1 : 0);

    float metalness = material.maps[MATERIAL_MAP_METALNESS].value;
    float roughness = material.maps[MATERIAL_MAP_ROUGHNESS].value;
    Color colEmissive = material.maps[MATERIAL_MAP_EMISSION].color;
    float emissiveIntensity = 0.0f;

    for (const auto& slot : materialSlotOverrides)
    {
        bool match = (slot.Target == MaterialSlotTarget::MeshIndex && slot.Index == meshIndex) ||
                     (slot.Target == MaterialSlotTarget::MaterialIndex && slot.Index == model.meshMaterial[meshIndex]);
        if (match)
        {
            emissiveIntensity = slot.Material.EmissiveIntensity;
            if (slot.Material.OverrideEmissive) colEmissive = slot.Material.EmissiveColor;
            metalness = slot.Material.Metalness;
            roughness = slot.Material.Roughness;
            break;
        }
    }

    activeShader->SetFloat("metalness", metalness);
    activeShader->SetFloat("roughness", roughness);
    if (emissiveIntensity == 0.0f && (colEmissive.r > 0 || colEmissive.g > 0 || colEmissive.b > 0)) emissiveIntensity = 1.0f;
    activeShader->SetColor("colEmissive", colEmissive);
    activeShader->SetFloat("emissiveIntensity", emissiveIntensity);
}
} // namespace CHEngine
