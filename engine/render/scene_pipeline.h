#ifndef CH_SCENE_PIPELINE_H
#define CH_SCENE_PIPELINE_H

namespace CHEngine
{
class Scene;
struct DebugRenderFlags;

class ScenePipeline
{
public:
    static void RenderScene(Scene *scene, const DebugRenderFlags *debugFlags = nullptr);
};
} // namespace CHEngine

#endif // CH_SCENE_PIPELINE_H
