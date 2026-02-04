#include "render_command.h"
#include "rlgl.h"

namespace CHEngine
{
    void RenderCommand::Init()
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

    void RenderCommand::DrawLine(Vector3 start, Vector3 end, Color color)
    {
        DrawLine3D(start, end, color);
    }

    void RenderCommand::DrawGrid(int slices, float spacing)
    {
        ::DrawGrid(slices, spacing);
    }

    void RenderCommand::SetBlendMode(int mode)
    {
        rlSetBlendMode(mode);
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
