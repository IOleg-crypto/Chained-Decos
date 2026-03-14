#include "render_command.h"
#include "rlgl.h"

namespace CHEngine
{

std::unique_ptr<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

void RenderCommand::Initialize()
{
    s_RendererAPI->Init();
}

void RenderCommand::Shutdown()
{
}

// Clear and SetViewport are now inline in header

void RenderCommand::DrawLine(Vector3 startPosition, Vector3 endPosition, Color color)
{
    DrawLine3D(startPosition, endPosition, color);
}

void RenderCommand::DrawGrid(int sliceCount, float spacing)
{
    ::DrawGrid(sliceCount, spacing);
}

void RenderCommand::SetBlendMode(int blendMode)
{
    rlSetBlendMode(blendMode);
}

void RenderCommand::EnableDepthTest()
{
    rlEnableDepthTest();
}

void RenderCommand::DisableDepthTest()
{
    rlDisableDepthTest();
}

void RenderCommand::EnableBackfaceCulling()
{
    rlEnableBackfaceCulling();
}

void RenderCommand::DisableBackfaceCulling()
{
    rlDisableBackfaceCulling();
}

void RenderCommand::EnableDepthMask()
{
    rlEnableDepthMask();
}

void RenderCommand::DisableDepthMask()
{
    rlDisableDepthMask();
}
} // namespace CHEngine
