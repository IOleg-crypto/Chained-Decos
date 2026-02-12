#ifndef CH_SCENE_RENDERER_H
#define CH_SCENE_RENDERER_H

#include "engine/scene/scene.h"
#include "engine/graphics/render.h"

namespace CHEngine
{
    class SceneRenderer
    {
    public:
        static void RenderScene(Scene* scene, const Camera3D& camera, Timestep timestep, const DebugRenderFlags* debugFlags = nullptr);
        
        // Internal methods used by RenderScene
        static void RenderModels(Scene* scene, Timestep timestep);
        static void RenderDebug(Scene* scene, const DebugRenderFlags* debugFlags);
        static void RenderEditorIcons(Scene* scene, const Camera3D& camera);
        static void RenderSprites(Scene* scene);
    };
}

#endif // CH_SCENE_RENDERER_H
