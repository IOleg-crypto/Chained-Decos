#ifndef CH_SCENE_RENDERER_H
#define CH_SCENE_RENDERER_H

#include "engine/scene/scene.h"
#include "engine/graphics/renderer.h"

namespace CHEngine
{
    class SceneRenderer
    {
    public:
        SceneRenderer() = default;
        ~SceneRenderer() = default;
    public:
        void RenderScene(Scene* scene, const Camera3D& camera, Timestep timestep, const DebugRenderFlags* debugFlags = nullptr);
        
        // Internal methods used by RenderScene
        void RenderModels(Scene* scene, Timestep timestep);
        void RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags);
        void RenderEditorIcons(Scene* scene, const Camera3D& camera);
        void RenderSprites(Scene* scene);

    private:
        void DrawColliderDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);
        void DrawCollisionModelBoxDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);
        void DrawSpawnDebug(entt::registry& registry, const DebugRenderFlags* debugFlags);
    };
}

#endif // CH_SCENE_RENDERER_H
