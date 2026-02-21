#ifndef CH_SCENE_RENDERER_H
#define CH_SCENE_RENDERER_H

#include "engine/graphics/renderer.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
class SceneRenderer
{
public:
    SceneRenderer() = default;
    ~SceneRenderer() = default;

public:
    void RenderScene(Scene* scene, const Camera3D& camera, Timestep timestep,
                     const DebugRenderFlags* debugFlags = nullptr);

    // Internal methods used by RenderScene
    void RenderModels(Scene* scene, Timestep timestep);
    void RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags);
    void RenderEditorIcons(Scene* scene, const Camera3D& camera);
    void RenderSprites(Scene* scene);

private:
    void DrawColliderDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);
    void DrawCollisionModelBoxDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);
    void DrawSpawnDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);

    static Matrix GetWorldTransform(entt::registry& registry, entt::entity entity);
    static Vector3 GetWorldPosition(entt::registry& registry, entt::entity entity);
    static void RenderBVHNode(const BVH* bvh, uint32_t nodeIndex, const Matrix& transform, Color color, int depth = 0);
};
} // namespace CHEngine

#endif // CH_SCENE_RENDERER_H
