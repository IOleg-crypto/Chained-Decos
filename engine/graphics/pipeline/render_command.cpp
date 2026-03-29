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

void RenderCommand::DrawLine(glm::vec3 startPosition, glm::vec3 endPosition, Color color)
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

} // namespace CHEngine
