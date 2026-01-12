#include "render_command.h"
#include <rlgl.h>

namespace CHEngine
{
void RenderCommand::Init()
{
    rlEnableDepthTest();
}

void RenderCommand::SetClearColor(const Color &color)
{
    // Raylib handles clear color in ClearBackground
}

void RenderCommand::Clear()
{
    // Normally handled by BeginDrawing/ClearBackground in Raylib
    // But we can manually clear if needed
    rlClearColor(0, 0, 0, 255);
    rlClearScreenBuffers();
}

void RenderCommand::SetViewport(int x, int y, int width, int height)
{
    rlViewport(x, y, width, height);
}

void RenderCommand::DrawIndexed(unsigned int count)
{
    // Placeholder for direct rlgl draw calls if we move away from high-level DrawModel
}

void RenderCommand::SetDepthTest(bool enabled)
{
    if (enabled)
        rlEnableDepthTest();
    else
        rlDisableDepthTest();
}

void RenderCommand::SetDepthWrite(bool enabled)
{
    if (enabled)
        rlEnableDepthMask();
    else
        rlDisableDepthMask();
}

void RenderCommand::SetBlending(bool enabled)
{
    if (enabled)
        rlEnableColorBlend();
    else
        rlDisableColorBlend();
}

} // namespace CHEngine
