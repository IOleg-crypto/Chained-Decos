#include "render_command.h"
#include <glad/gl.h>

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

void RenderCommand::DrawLine(Vector3 startPosition, Vector3 endPosition, Color color)
{
    // TODO: Modern OpenGL line drawing (VAO/VBO batching)
    /*
    glBegin(GL_LINES);
    glColor4ub(color.r, color.g, color.b, color.a);
    glVertex3f(startPosition.x, startPosition.y, startPosition.z);
    glVertex3f(endPosition.x, endPosition.y, endPosition.z);
    glEnd();
    */
}

void RenderCommand::DrawGrid(int sliceCount, float spacing)
{
    // TODO: Modern OpenGL grid drawing
    /*
    float halfSize = (float)sliceCount * spacing / 2.0f;
    glBegin(GL_LINES);
    glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
    for (int i = 0; i <= sliceCount; i++)
    {
        float pos = -halfSize + (float)i * spacing;
        glVertex3f(pos, 0, -halfSize);
        glVertex3f(pos, 0, halfSize);
        glVertex3f(-halfSize, 0, pos);
        glVertex3f(halfSize, 0, pos);
    }
    glEnd();
    */
}

void RenderCommand::SetBlendMode(int blendMode)
{
    s_RendererAPI->SetBlendMode(blendMode != 0);
}

void RenderCommand::EnableDepthTest()
{
    s_RendererAPI->SetDepthTest(true);
}

void RenderCommand::DisableDepthTest()
{
    s_RendererAPI->SetDepthTest(false);
}

void RenderCommand::EnableBackfaceCulling()
{
    s_RendererAPI->SetCulling(true);
}

void RenderCommand::DisableBackfaceCulling()
{
    s_RendererAPI->SetCulling(false);
}

void RenderCommand::EnableDepthMask()
{
    s_RendererAPI->SetDepthMask(true);
}

void RenderCommand::DisableDepthMask()
{
    s_RendererAPI->SetDepthMask(false);
}
} // namespace CHEngine
