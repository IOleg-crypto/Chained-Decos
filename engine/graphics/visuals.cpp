#include "visuals.h"
#include "api_context.h"
#include "draw_command.h"
#include "scene_pipeline.h"


namespace CHEngine
{
void Visuals::Init()
{
    APIContext::Init();
}

void Visuals::Shutdown()
{
    APIContext::Shutdown();
}

void Visuals::DrawScene(Scene *scene, const Camera3D &camera, const DebugRenderFlags *debugFlags)
{
    ScenePipeline::Render(scene, camera, debugFlags);
}

void Visuals::Clear(Color color)
{
    DrawCommand::Clear(color);
}
} // namespace CHEngine
