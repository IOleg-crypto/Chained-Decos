#ifndef CH_SCENE_RENDERER_H
#define CH_SCENE_RENDERER_H

#include <raylib.h>

#include "engine/renderer/render.h"
#include "engine/renderer/render_state.h"
#include "engine/scene/scene.h"

namespace CHEngine
{

class SceneRender
{
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const RenderState &state);
    static void EndScene();
    static void SubmitScene(const RenderState &state);

    // Snapshot helper (runs on simulation thread)
    static void CreateSnapshot(Scene *scene, const Camera3D &camera, RenderState &outState,
                               float alpha, const DebugRenderFlags *debugFlags = nullptr);

private:
    static void RenderOpaquePass();
    static void RenderTransparentPass();
    static void RenderDebugPass();
};
} // namespace CHEngine

#endif // CH_SCENE_RENDERER_H
