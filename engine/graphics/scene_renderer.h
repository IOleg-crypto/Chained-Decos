#ifndef CH_SCENE_RENDERER_H
#define CH_SCENE_RENDERER_H

#include "engine/graphics/renderer.h"
#include "engine/scene/scene.h"
#include "engine/scene/components/animation_component.h"
#include "engine/scene/components/physics_component.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace CHEngine
{
struct Frustum;

class SceneRenderer
{
public:
    SceneRenderer() = default;
    ~SceneRenderer() = default;

public:
    void RenderScene(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep,
                     const DebugRenderFlags* debugFlags = nullptr);

    // Internal methods used by RenderScene
    void RenderModels(Scene* scene, const Camera3D& camera, float nearClip, float farClip, Timestep timestep);
    void RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags);
    void RenderEditorIcons(Scene* scene, const Camera3D& camera);
    void RenderSprites(Scene* scene);

private:
    struct AnimatedEntry
    {
        std::shared_ptr<class ModelAsset> asset;
        Matrix worldTransform;
        std::vector<MaterialSlot> materials;
        std::shared_ptr<class ShaderAsset> shaderOverride;
        std::vector<ShaderUniform> customUniforms;
        AnimationComponent animation;
    };

    struct InstanceKey
    {
        size_t Hash = 0;
        InstanceKey(const std::string& path, const std::vector<MaterialSlot>& mats);
        bool operator<(const InstanceKey& o) const { return Hash < o.Hash; }
        bool operator==(const InstanceKey& o) const { return Hash == o.Hash; }
    };

    struct InstanceKeyHash
    {
        size_t operator()(const InstanceKey& k) const { return k.Hash; }
    };

    struct InstanceGroup
    {
        std::shared_ptr<class ModelAsset> asset;
        std::vector<Matrix> transforms;
        std::vector<MaterialSlot> materials;
    };

private:
    void PrepareLights(entt::registry& registry, const Frustum& frustum);
    void CollectRenderItems(entt::registry& registry, const Frustum& frustum,
                            std::vector<AnimatedEntry>& animatedEntries,
                            std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups);
    void DrawAnimatedEntities(const std::vector<AnimatedEntry>& animatedEntries);
    void DrawStaticEntities(std::unordered_map<InstanceKey, InstanceGroup, InstanceKeyHash>& instanceGroups);

    void DrawColliderDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);
    void DrawCollisionModelBoxDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);
    void DrawSpawnDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);

    static BoundingBox CalculateColliderWorldAABB(const ColliderComponent& collider, const Matrix& worldTransform);

    static Matrix GetWorldTransform(entt::registry& registry, entt::entity entity);
    static Vector3 GetWorldPosition(entt::registry& registry, entt::entity entity);
    static void RenderBVHNode(const class BVH* bvh, uint32_t nodeIndex, const Matrix& transform, Color color, int depth = 0);
};
} // namespace CHEngine

#endif // CH_SCENE_RENDERER_H
