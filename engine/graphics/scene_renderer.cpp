#include "engine/graphics/renderer.h"
#include "engine/graphics/model_asset.h"
#include "engine/graphics/shader_asset.h"
#include "scene_renderer.h"
#include "engine/scene/components.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/core/profiler.h"
#include "engine/core/assert.h"
#include "engine/graphics/renderer2d.h"
#include "engine/scene/project.h"
#include "render_command.h"
#include <raymath.h>
#include <rlgl.h>

namespace CHEngine
{
    static Matrix GetWorldTransform(entt::registry& registry, entt::entity entity)
    {
        Matrix transform = MatrixIdentity();
        if (registry.all_of<TransformComponent>(entity))
        {
            transform = registry.get<TransformComponent>(entity).GetTransform();
        }

        if (registry.all_of<HierarchyComponent>(entity))
        {
            entt::entity parent = registry.get<HierarchyComponent>(entity).Parent;
            if (parent != entt::null)
            {
                transform = MatrixMultiply(transform, GetWorldTransform(registry, parent));
            }
        }
        return transform;
    }

    static Vector3 GetWorldPosition(entt::registry& registry, entt::entity entity)
    {
        Matrix transform = GetWorldTransform(registry, entity);
        return { transform.m12, transform.m13, transform.m14 };
    }

    void SceneRenderer::RenderScene(Scene* scene, const Camera3D& camera, Timestep timestep, const DebugRenderFlags* debugFlags)
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

        if (environment)
        {
            Renderer::Get().ApplyEnvironment(environment->GetSettings());
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
                    if (!warned) {
                        CH_CORE_WARN("SceneRenderer: Environment exists but Skybox.TexturePath is empty!");
                        warned = true;
                    }
                }
                Renderer::Get().DrawSkybox(environment->GetSettings().Skybox, camera);
            }
            else
            {
                static bool warned = false;
                if (!warned) {
                    CH_CORE_WARN("SceneRenderer: No environment asset for scene!");
                    warned = true;
                }
            }

            RenderModels(scene, timestep);
            
            if (debugFlags)
                RenderDebug(scene, debugFlags);
                
            RenderSprites(scene);

            RenderEditorIcons(scene, camera);
        }
        Renderer::Get().EndScene();
    }

    void SceneRenderer::RenderModels(Scene* scene, Timestep timestep)
    {
        auto& registry = scene->GetRegistry();
        auto view = registry.view<TransformComponent, ModelComponent>();

        // 1. Collect Lights
        Renderer::Get().ClearLights();
        
        int lightCount = 0;
        auto lightView = registry.view<LightComponent>();
        for (auto entity : lightView)
        {
            if (lightCount >= RendererData::MaxLights) break;
            auto& light = lightView.get<LightComponent>(entity);
            Vector3 worldPos = GetWorldPosition(registry, entity);
            
            RenderLight rl;
            rl.color = light.LightColor;
            rl.position = worldPos;
            rl.intensity = (light.Intensity <= 0.0f) ? 1.0f : light.Intensity;
            rl.radius = light.Radius;
            rl.innerCutoff = light.InnerCutoff;
            rl.outerCutoff = light.OuterCutoff;
            rl.type = (int)light.Type;
            rl.enabled = true;

            if (light.Type == LightType::Spot)
            {
                Matrix worldTransform = GetWorldTransform(registry, entity);
                Vector3 worldDir = Vector3Transform({ 0, -1, 0 }, worldTransform);
                rl.direction = Vector3Normalize(Vector3Subtract(worldDir, worldPos));
            }

            Renderer::Get().SetLight(lightCount++, rl);
        }

        // 2. Render Models
        for (auto entity : view)
        {
            auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);
            
            // Check for deferred texture updates
            if (model.Asset) model.Asset->OnUpdate();

            std::shared_ptr<ShaderAsset> shaderOverride = nullptr;
            std::vector<ShaderUniform> customUniforms;

            if (registry.all_of<ShaderComponent>(entity))
            {
                auto& shaderComp = registry.get<ShaderComponent>(entity);
                if (shaderComp.Enabled && !shaderComp.ShaderPath.empty())
                {
                    if (Project::GetActive())
                    {
                        shaderOverride = Project::GetActive()->GetAssetManager()->Get<ShaderAsset>(shaderComp.ShaderPath);
                        customUniforms = shaderComp.Uniforms;
                    }
                }
            }

            // Use World Transform for correct hierarchy visualization
            Matrix worldTransform = GetWorldTransform(registry, entity);

            if (registry.all_of<AnimationComponent>(entity))
            {
                auto& animation = registry.get<AnimationComponent>(entity);
                Renderer::Get().DrawModel(model.Asset, worldTransform, model.Materials, animation.CurrentAnimationIndex, animation.CurrentFrame, shaderOverride, customUniforms);
            }
            else
            {
                Renderer::Get().DrawModel(model.Asset, worldTransform, model.Materials, 0, 0, shaderOverride, customUniforms);
            }
        }
    }

    static void RenderBVHNode(const BVH* bvh, uint32_t nodeIndex, const Matrix& transform, Color color, int depth = 0)
    {
        const auto& nodes = bvh->GetNodes();
        if (nodeIndex >= nodes.size()) return;

        const auto& node = nodes[nodeIndex];
        bool isLeaf = node.IsLeaf();

        // Only draw root or leaves to reduce clutter in the editor
        if (depth == 0 || isLeaf)
        {
            Color nodeColor = isLeaf ? ORANGE : color;
            
            Vector3 center = Vector3Scale(Vector3Add(node.Min, node.Max), 0.5f);
            Vector3 size = Vector3Subtract(node.Max, node.Min);

            // Transform is already in world space
            Matrix nodeTransform = MatrixMultiply(MatrixTranslate(center.x, center.y, center.z), transform);
            Renderer::Get().DrawCubeWires(nodeTransform, size, nodeColor);
        }

        if (!isLeaf && depth < 20) // Increased depth for more detail if needed
        {
            RenderBVHNode(bvh, node.LeftOrFirst, transform, color, depth + 1);
            RenderBVHNode(bvh, node.LeftOrFirst + 1, transform, color, depth + 1);
        }
    }

    void SceneRenderer::RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags)
    {
        if (!debugFlags) return;
        auto& registry = scene->GetRegistry();

        // Disable depth test to ensure colliders are visible even inside models
        rlDisableDepthTest();

        // 1. Draw Colliders (OBB / Shapes)
        if (debugFlags->DrawColliders)
        {
            auto view = registry.view<TransformComponent, ColliderComponent>();
            for (auto entity : view)
            {
                auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);
                if (!collider.Enabled) continue;

                Matrix worldTransform = GetWorldTransform(registry, entity);
                Color color = GREEN;

                if (collider.Type == ColliderType::Mesh && collider.BVHRoot)
                {
                    RenderBVHNode(collider.BVHRoot.get(), 0, worldTransform, color);
                }
                else if (collider.Type == ColliderType::Box)
                {
                    Vector3 center = Vector3Add(collider.Offset, Vector3Scale(collider.Size, 0.5f));
                    Matrix colliderTransform = MatrixMultiply(MatrixTranslate(center.x, center.y, center.z), worldTransform);
                    Renderer::Get().DrawCubeWires(colliderTransform, collider.Size, color);
                }
                else if (collider.Type == ColliderType::Capsule)
                {
                    Matrix colliderTransform = MatrixMultiply(MatrixTranslate(collider.Offset.x, collider.Offset.y, collider.Offset.z), worldTransform);
                    Renderer::Get().DrawCapsuleWires(colliderTransform, collider.Radius, collider.Height, color);
                }
                else if (collider.Type == ColliderType::Sphere)
                {
                    Matrix colliderTransform = MatrixMultiply(MatrixTranslate(collider.Offset.x, collider.Offset.y, collider.Offset.z), worldTransform);
                    Renderer::Get().DrawSphereWires(colliderTransform, collider.Radius, color);
                }
            }
        }

        // 1.5 Draw World AABBs (Axis-Aligned)
        if (debugFlags->DrawAABB)
        {
            auto view = registry.view<TransformComponent, ColliderComponent>();
            for (auto entity : view)
            {
                auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);
                if (!collider.Enabled) continue;

                Matrix worldTransform = GetWorldTransform(registry, entity);
                BoundingBox worldAABB = {{0,0,0}, {0,0,0}};
                bool hasBounds = false;

                if (collider.Type == ColliderType::Mesh && collider.BVHRoot)
                {
                    const auto& nodes = collider.BVHRoot->GetNodes();
                    if (!nodes.empty())
                    {
                        Vector3 corners[8] = {
                            Vector3Transform({nodes[0].Min.x, nodes[0].Min.y, nodes[0].Min.z}, worldTransform),
                            Vector3Transform({nodes[0].Max.x, nodes[0].Min.y, nodes[0].Min.z}, worldTransform),
                            Vector3Transform({nodes[0].Min.x, nodes[0].Max.y, nodes[0].Min.z}, worldTransform),
                            Vector3Transform({nodes[0].Max.x, nodes[0].Max.y, nodes[0].Min.z}, worldTransform),
                            Vector3Transform({nodes[0].Min.x, nodes[0].Min.y, nodes[0].Max.z}, worldTransform),
                            Vector3Transform({nodes[0].Max.x, nodes[0].Min.y, nodes[0].Max.z}, worldTransform),
                            Vector3Transform({nodes[0].Min.x, nodes[0].Max.y, nodes[0].Max.z}, worldTransform),
                            Vector3Transform({nodes[0].Max.x, nodes[0].Max.y, nodes[0].Max.z}, worldTransform)
                        };
                        worldAABB.min = corners[0]; worldAABB.max = corners[0];
                        for(int i=1; i<8; i++) {
                            worldAABB.min = Vector3Min(worldAABB.min, corners[i]);
                            worldAABB.max = Vector3Max(worldAABB.max, corners[i]);
                        }
                        hasBounds = true;
                    }
                }
                else if (collider.Type == ColliderType::Box)
                {
                    Vector3 corners[8] = {
                         Vector3Transform(collider.Offset, worldTransform),
                         Vector3Transform(Vector3Add(collider.Offset, {collider.Size.x, 0, 0}), worldTransform),
                         Vector3Transform(Vector3Add(collider.Offset, {0, collider.Size.y, 0}), worldTransform),
                         Vector3Transform(Vector3Add(collider.Offset, {collider.Size.x, collider.Size.y, 0}), worldTransform),
                         Vector3Transform(Vector3Add(collider.Offset, {0, 0, collider.Size.z}), worldTransform),
                         Vector3Transform(Vector3Add(collider.Offset, {collider.Size.x, 0, collider.Size.z}), worldTransform),
                         Vector3Transform(Vector3Add(collider.Offset, {0, collider.Size.y, collider.Size.z}), worldTransform),
                         Vector3Transform(Vector3Add(collider.Offset, {collider.Size.x, collider.Size.y, collider.Size.z}), worldTransform)
                    };
                    worldAABB.min = corners[0]; worldAABB.max = corners[0];
                    for(int i=1; i<8; i++) {
                        worldAABB.min = Vector3Min(worldAABB.min, corners[i]);
                        worldAABB.max = Vector3Max(worldAABB.max, corners[i]);
                    }
                    hasBounds = true;
                }
                else if (collider.Type == ColliderType::Capsule)
                {
                    float halfSeg = fmaxf(0.0f, collider.Height * 0.5f - collider.Radius);
                    Vector3 worldA = Vector3Transform(Vector3Add(collider.Offset, {0, -halfSeg, 0}), worldTransform);
                    Vector3 worldB = Vector3Transform(Vector3Add(collider.Offset, {0, halfSeg, 0}), worldTransform);
                    
                    float r = collider.Radius;
                    worldAABB.min = Vector3Subtract(Vector3Min(worldA, worldB), {r, r, r});
                    worldAABB.max = Vector3Add(Vector3Max(worldA, worldB), {r, r, r});
                    hasBounds = true;
                }
                else if (collider.Type == ColliderType::Sphere)
                {
                    Vector3 worldPos = Vector3Transform(collider.Offset, worldTransform);
                    float r = collider.Radius;
                    worldAABB.min = Vector3Subtract(worldPos, {r, r, r});
                    worldAABB.max = Vector3Add(worldPos, {r, r, r});
                    hasBounds = true;
                }

                if (hasBounds)
                {
                    Vector3 center = Vector3Scale(Vector3Add(worldAABB.min, worldAABB.max), 0.5f);
                    Vector3 size = Vector3Subtract(worldAABB.max, worldAABB.min);
                    Renderer::Get().DrawCubeWires(MatrixTranslate(center.x, center.y, center.z), size, RED);
                }
            }
        }

        // 2. Draw Spawn Zones
        if (debugFlags->DrawSpawnZones)
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

        // 3. Draw Grid
        if (debugFlags->DrawGrid && scene->GetSettings().Mode == BackgroundMode::Environment3D)
        {
            Renderer::Get().DrawGrid(20, 1.0f);
        }

        rlEnableDepthTest();
    }

    void SceneRenderer::RenderEditorIcons(Scene* scene, const Camera3D& camera)
    {
        auto& registry = scene->GetRegistry();
        auto& state = Renderer::Get().GetData();
        auto assetManager = Project::GetActive() ? Project::GetActive()->GetAssetManager() : nullptr;

        if (state.LightIcon.id == 0 && assetManager) {
            auto texture = assetManager->Get<TextureAsset>("engine/resources/icons/light_bulb.png");
            if (texture && texture->IsReady()) state.LightIcon = texture->GetTexture();
        }
        if (state.SpawnIcon.id == 0 && assetManager) {
            auto texture = assetManager->Get<TextureAsset>("engine/resources/icons/leaf_icon.png");
            if (texture && texture->IsReady()) state.SpawnIcon = texture->GetTexture();
        }
        if (state.CameraIcon.id == 0 && assetManager) {
            auto texture = assetManager->Get<TextureAsset>("engine/resources/icons/camera_icon.jpg");
            if (texture && texture->IsReady()) state.CameraIcon = texture->GetTexture();
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
        for (auto entity : view) sortedEntities.push_back(entity);

        if (sortedEntities.empty()) return;

        std::sort(sortedEntities.begin(), sortedEntities.end(), [&](entt::entity a, entt::entity b) {
            return view.get<SpriteComponent>(a).ZOrder < view.get<SpriteComponent>(b).ZOrder;
        });

        Renderer2D::Get().BeginCanvas();
        for (auto entityID : sortedEntities)
        {         
            auto& sprite = view.get<SpriteComponent>(entityID);

            if (sprite.TexturePath.empty()) continue;

            if (!sprite.Texture && Project::GetActive())
                sprite.Texture = Project::GetActive()->GetAssetManager()->Get<TextureAsset>(sprite.TexturePath);

            Vector3 worldPos = GetWorldPosition(registry, entityID);
            
            Renderer2D::Get().DrawSprite(Vector2{worldPos.x, worldPos.y},
                                    Vector2{1.0f, 1.0f},
                                    0.0f,
                                    sprite.Texture, sprite.Tint);
        }
        Renderer2D::Get().EndCanvas();
    }
}
