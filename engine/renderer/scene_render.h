#ifndef CH_SCENE_RENDERER_H
#define CH_SCENE_RENDERER_H

#include <raylib.h>

#include "engine/renderer/render.h"
#include "engine/scene/scene.h"


namespace CHEngine
{

class SceneRender
{
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(Scene *scene, const Camera3D &camera);
    static void EndScene();

    static void SubmitScene(Scene *scene, const DebugRenderFlags *debugFlags = nullptr);

private:
    static void RenderOpaquePass();
    static void RenderTransparentPass();
    static void RenderDebugPass();
};
} // namespace CHEngine

#endif // CH_SCENE_RENDERER_H
