#include "render_command.h"
#include "rlgl.h"

namespace CHEngine
{
    void RenderCommand::Initialize()
    {
    }

    void RenderCommand::Shutdown()
    {
    }

    void RenderCommand::Clear(Color color)
    {
        ClearBackground(color);
    }

    void RenderCommand::SetViewport(int x, int y, int width, int height)
    {
        rlViewport(x, y, width, height);
    }

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
}
