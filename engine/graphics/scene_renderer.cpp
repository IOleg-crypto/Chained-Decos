#include "engine/graphics/renderer.h"
#include "engine/graphics/model_asset.h"
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
    void SceneRenderer::RenderScene(Scene* scene, const Camera3D& camera, Timestep timestep, const DebugRenderFlags* debugFlags)
    {
        CH_PROFILE_FUNCTION();
        CH_CORE_ASSERT(Renderer::IsInitialized(), "Renderer not initialized!");
        CH_CORE_ASSERT(scene, "Scene is null!");
        
        // 1. Environmental setup
        auto environment = scene->GetSettings().Environment;
        
        // Fallback to project environment if not set in scene
        if (!environment && Project::GetActive())
            environment = Project::GetActive()->GetEnvironment();

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
        
        float targetFPS = 30.0f;
        if (Project::GetActive()){
            targetFPS = Project::GetActive()->GetConfig().Animation.TargetFPS;
        }
        float frameTime = 1.0f / (targetFPS > 0 ? targetFPS : 30.0f);
        

        for (auto entity : view)
        {
            auto [transform, model] = view.get<TransformComponent, ModelComponent>(entity);
            
            // Check for deferred texture updates
            if (model.Asset) model.Asset->OnUpdate();

            if (registry.all_of<AnimationComponent>(entity))
            {
                auto& animation = registry.get<AnimationComponent>(entity);
                Renderer::Get().DrawModel(model.Asset, transform.GetTransform(), {}, animation.CurrentAnimationIndex, animation.CurrentFrame);
            }
            else
            {
                Renderer::Get().DrawModel(model.Asset, transform.GetTransform());
            }
        }
        
    }

    static void RenderBVHNode(const BVH* bvh, uint32_t nodeIndex, const Matrix& transform, Color color, int depth = 0)
    {
        const auto& nodes = bvh->GetNodes();
        if (nodeIndex >= nodes.size()) return;

        const auto& node = nodes[nodeIndex];

        // Variation of color based on depth
        Color nodeColor = color;
        if (depth > 0)
        {
            float tint = 1.0f - (float)depth / 8.0f;
            if (tint < 0.2f) tint = 0.2f;
            nodeColor.r = (unsigned char)(color.r * tint);
            nodeColor.g = (unsigned char)(color.g * tint);
            nodeColor.b = (unsigned char)(color.b * tint);
        }

        Vector3 center = Vector3Scale(Vector3Add(node.Min, node.Max), 0.5f);
        Vector3 size = Vector3Subtract(node.Max, node.Min);

        // Transform is already in world space
        Matrix nodeTransform = MatrixMultiply(transform, MatrixTranslate(center.x, center.y, center.z));
        Renderer::Get().DrawCubeWires(nodeTransform, size, nodeColor);

        if (!node.IsLeaf() && depth < 8) // Limit depth for performance and clarity
        {
            RenderBVHNode(bvh, node.LeftOrFirst, transform, color, depth + 1);
            RenderBVHNode(bvh, node.LeftOrFirst + 1, transform, color, depth + 1);
        }
    }

    void SceneRenderer::RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags)
    {
        if (!debugFlags) return;
        auto& registry = scene->GetRegistry();

        // 1. Draw Colliders
        if (debugFlags->DrawColliders)
        {
            auto view = registry.view<TransformComponent, ColliderComponent>();
            for (auto entity : view)
            {
                auto [transform, collider] = view.get<TransformComponent, ColliderComponent>(entity);
                
                Color color = GREEN;
                if (collider.Type == ColliderType::Mesh && collider.BVHRoot)
                {
                    // For Mesh Colliders, the BVH is already in local model space.
                    // We only need the entity world transform.
                    Matrix worldTransform = transform.GetTransform();
                    
                    if (debugFlags->DrawAABB)
                    {
                        RenderBVHNode(collider.BVHRoot.get(), 0, worldTransform, color);
                    }
                    else
                    {
                        // Draw just the collision mesh wires (more optimized than individual DrawLine3D)
                        // Note: For extreme performance we should store a Raylib Mesh in the collider, 
                        // but for now we draw a sampling or just the root bounds.
                        
                        // Root bounds of the BVH
                        const auto& nodes = collider.BVHRoot->GetNodes();
                        if (!nodes.empty())
                        {
                            Vector3 center = Vector3Scale(Vector3Add(nodes[0].Min, nodes[0].Max), 0.5f);
                            Vector3 size = Vector3Subtract(nodes[0].Max, nodes[0].Min);
                            Matrix rootTransform = MatrixMultiply(worldTransform, MatrixTranslate(center.x, center.y, center.z));
                            Renderer::Get().DrawCubeWires(rootTransform, size, color);
                        }

                        // Also draw a few triangles if close? 
                        // For now, let's just draw the mesh wires but only if specifically requested or simple.
                        // To solve the user's "render" bug (clutter/performance), we stop drawing ALL triangles by default.
                    }
                }
                else
                {
                    Matrix colliderTransform = MatrixMultiply(transform.GetTransform(), MatrixTranslate(collider.Offset.x, collider.Offset.y, collider.Offset.z));
                    // Draw oriented bounding box (OBB)
                    Renderer::Get().DrawCubeWires(colliderTransform, collider.Size, color);
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
                    // Draw a more visible oriented box for the zone
                    Renderer::Get().DrawCubeWires(transform.GetTransform(), spawn.ZoneSize, {255, 255, 0, 200});
                }
            }
        }

        // 3. Draw Grid
        if (debugFlags->DrawGrid && scene->GetSettings().Mode == BackgroundMode::Environment3D)
        {
            Renderer::Get().DrawGrid(20, 1.0f);
        }
    }

    void SceneRenderer::RenderEditorIcons(Scene* scene, const Camera3D& camera)
    {
        auto& registry = scene->GetRegistry();
        auto& state = Renderer::Get().GetData();
        auto assetManager = Project::GetActive() ? Project::GetActive()->GetAssetManager() : nullptr;

        // Ensure we have icons (fallback to AssetManager if Renderer::Get().Initialize failed sync load)
        if (state.LightIcon.id == 0 && assetManager) {
            auto texture = assetManager->Get<TextureAsset>(PROJECT_ROOT_DIR "/engine/resources/icons/light_bulb.png");
            if (texture && texture->IsReady()) state.LightIcon = texture->GetTexture();
        }
        if (state.SpawnIcon.id == 0 && assetManager) {
            auto texture = assetManager->Get<TextureAsset>(PROJECT_ROOT_DIR "/engine/resources/icons/leaf_icon.png");
            if (texture && texture->IsReady()) state.SpawnIcon = texture->GetTexture();
        }
        if (state.CameraIcon.id == 0 && assetManager) {
            auto texture = assetManager->Get<TextureAsset>(PROJECT_ROOT_DIR "/engine/resources/icons/icon_camera.jpg");
            if (texture && texture->IsReady()) state.CameraIcon = texture->GetTexture();
        }

        // Disable depth testing for icons to make them always visible in editor
        rlDisableDepthTest();

        // 1. Light Icons
        {
            auto view = registry.view<TransformComponent, PointLightComponent>();
            for (auto entity : view)
            {
                auto& transform = view.get<TransformComponent>(entity);
                Renderer::Get().DrawBillboard(camera, state.LightIcon, transform.Translation, 1.5f, WHITE);
            }
        }

        // 2. Spawn Zone Icons
        {
            auto view = registry.view<TransformComponent, SpawnComponent>();
            for (auto entity : view)
            {
                auto& transform = view.get<TransformComponent>(entity);
                Renderer::Get().DrawBillboard(camera, state.SpawnIcon, transform.Translation, 1.5f, WHITE);
            }
        }

        // 3. Camera Icons
        {
            auto view = registry.view<TransformComponent, CameraComponent>();
            for (auto entity : view)
            {
                auto& transform = view.get<TransformComponent>(entity);
                Renderer::Get().DrawBillboard(camera, state.CameraIcon, transform.Translation, 1.5f, WHITE);
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

        // Sort by ZOrder
        std::vector<entt::entity> sortedEntities;
        for (auto entity : view) sortedEntities.push_back(entity);

        if (sortedEntities.empty()) return;

        std::sort(sortedEntities.begin(), sortedEntities.end(), [&](entt::entity a, entt::entity b) {
            return view.get<SpriteComponent>(a).ZOrder < view.get<SpriteComponent>(b).ZOrder;
        });

        Renderer2D::Get().BeginCanvas();
        for (auto entityID : sortedEntities)
        {
            auto& transform = view.get<TransformComponent>(entityID);
            auto& sprite = view.get<SpriteComponent>(entityID);

            if (sprite.TexturePath.empty()) continue;

            if (!sprite.Texture && Project::GetActive())
                sprite.Texture = Project::GetActive()->GetAssetManager()->Get<TextureAsset>(sprite.TexturePath);

            // Draw as 2D overlay
            Renderer2D::Get().DrawSprite(Vector2{transform.Translation.x, transform.Translation.y}, 
                                    Vector2{transform.Scale.x, transform.Scale.y}, 
                                    transform.Rotation.z, 
                                    sprite.Texture, sprite.Tint);
        }
        Renderer2D::Get().EndCanvas();
    }
}
