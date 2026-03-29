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

void OpenGLRendererAPI::SetCullMode(CullMode mode)
{
    if (mode == CullMode::None)
    {
        glDisable(GL_CULL_FACE);
    }
    else
    {
        glEnable(GL_CULL_FACE);
        switch (mode)
        {
            case CullMode::Front:        glCullFace(GL_FRONT); break;
            case CullMode::Back:         glCullFace(GL_BACK); break;
            case CullMode::FrontAndBack: glCullFace(GL_FRONT_AND_BACK); break;
        }
    }
}

void OpenGLRendererAPI::SetBlendMode(bool enabled)
{
    if (enabled) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
}

void OpenGLRendererAPI::SetBlendFunc(BlendFactor src, BlendFactor dst)
{
    auto toGL = [](BlendFactor factor) -> GLenum {
        switch (factor)
        {
            case BlendFactor::Zero:           return GL_ZERO;
            case BlendFactor::One:            return GL_ONE;
            case BlendFactor::SrcColor:       return GL_SRC_COLOR;
            case BlendFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
            case BlendFactor::DstColor:       return GL_DST_COLOR;
            case BlendFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
            case BlendFactor::SrcAlpha:       return GL_SRC_ALPHA;
            case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
            case BlendFactor::DstAlpha:       return GL_DST_ALPHA;
            case BlendFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
        }
        return GL_ONE;
    };
    glBlendFunc(toGL(src), toGL(dst));
}

void OpenGLRendererAPI::SetLineWidth(float width)
{
    glLineWidth(width);
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
