#include "opengl_renderer_api.h"
#include "engine/graphics/api/vertex_array.h"
#include <glad/gl.h>

namespace CHEngine
{

void OpenGLRendererAPI::Init()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
}

void OpenGLRendererAPI::SetViewport(int x, int y, int width, int height)
{
    glViewport(x, y, width, height);
}

void OpenGLRendererAPI::SetClearColor(const Color& color)
{
    m_ClearColor[0] = color.r / 255.0f;
    m_ClearColor[1] = color.g / 255.0f;
    m_ClearColor[2] = color.b / 255.0f;
    m_ClearColor[3] = color.a / 255.0f;
}

void OpenGLRendererAPI::Clear()
{
    glClearColor(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRendererAPI::SetDepthFunc(DepthFunc func)
{
    GLenum glFunc = GL_LESS;
    switch (func)
    {
        case DepthFunc::Never:    glFunc = GL_NEVER; break;
        case DepthFunc::Less:     glFunc = GL_LESS; break;
        case DepthFunc::Equal:    glFunc = GL_EQUAL; break;
        case DepthFunc::LEqual:   glFunc = GL_LEQUAL; break;
        case DepthFunc::Greater:  glFunc = GL_GREATER; break;
        case DepthFunc::NotEqual: glFunc = GL_NOTEQUAL; break;
        case DepthFunc::GEqual:   glFunc = GL_GEQUAL; break;
        case DepthFunc::Always:   glFunc = GL_ALWAYS; break;
    }
    glDepthFunc(glFunc);
}

void OpenGLRendererAPI::SetDepthTest(bool enabled)
{
    if (enabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
}

void OpenGLRendererAPI::SetDepthMask(bool enabled)
{
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}

void OpenGLRendererAPI::SetCulling(bool enabled)
{
    if (enabled) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
}

void OpenGLRendererAPI::SetBlendMode(bool enabled)
{
    if (enabled) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
}

void OpenGLRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
    uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
    glDrawElements(GL_TRIANGLES, (GLsizei)count, GL_UNSIGNED_INT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLRendererAPI::DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount)
{
    glDrawArrays(GL_LINES, 0, (GLsizei)vertexCount);
}

} // namespace CHEngine
