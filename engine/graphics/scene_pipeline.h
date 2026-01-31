#ifndef CH_SCENE_PIPELINE_H
#define CH_SCENE_PIPELINE_H

#include "engine/graphics/render_types.h"
#include "engine/scene/scene.h"


namespace CHEngine
{
class ScenePipeline
{
public:
    static void Render(Scene *scene, const Camera3D &camera,
                       const DebugRenderFlags *debugFlags = nullptr);

private:
    static void RenderSkybox(Scene *scene, const Camera3D &camera);
    static void RenderModels(Scene *scene);
    static void RenderDebug(Scene *scene, const DebugRenderFlags *debugFlags);
    static void RenderEditorIcons(Scene *scene, const Camera3D &camera);
};
} // namespace CHEngine

#endif // CH_SCENE_PIPELINE_H
