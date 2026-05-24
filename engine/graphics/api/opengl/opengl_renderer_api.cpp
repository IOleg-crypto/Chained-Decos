#include "opengl_renderer_api.h"
#include "engine/graphics/api/vertex_array.h"
#include <glad/gl.h>

namespace CHEngine
{

void OpenGLRendererAPI::Init()
{
// #ifdef CH_DEBUG
//     glEnable(GL_DEBUG_OUTPUT);
//     glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
//     glDebugMessageCallback(
//         [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
//            const void* userParam) {
//             switch (severity)
//             {
//             case GL_DEBUG_SEVERITY_HIGH:
//                 CH_CORE_ERROR("[OpenGL] {}", message);
//                 return;
//             case GL_DEBUG_SEVERITY_MEDIUM:
//                 CH_CORE_WARN("[OpenGL] {}", message);
//                 return;
//             case GL_DEBUG_SEVERITY_LOW:
//                 CH_CORE_INFO("[OpenGL] {}", message);
//                 return;
//             case GL_DEBUG_SEVERITY_NOTIFICATION:
//                 CH_CORE_TRACE("[OpenGL] {}", message);
//                 return;
//             }
//         },
//         nullptr);
// #endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_StateCache.Blend = true;
    m_StateCache.SrcBlend = BlendFactor::SrcAlpha;
    m_StateCache.DstBlend = BlendFactor::OneMinusSrcAlpha;

    glEnable(GL_DEPTH_TEST);
    m_StateCache.DepthTest = true;
    
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
    if (m_StateCache.DepthFunction == func) return;

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
    m_StateCache.DepthFunction = func;
}

void OpenGLRendererAPI::SetDepthTest(bool enabled)
{
    if (m_StateCache.DepthTest == enabled) return;

    if (enabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    
    m_StateCache.DepthTest = enabled;
}

void OpenGLRendererAPI::SetDepthMask(bool enabled)
{
    if (m_StateCache.DepthMask == enabled) return;

    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
    m_StateCache.DepthMask = enabled;
}

void OpenGLRendererAPI::SetCullMode(CullMode mode)
{
    if (m_StateCache.Cull == mode) return;

    if (mode == CullMode::None)
    {
        glDisable(GL_CULL_FACE);
    }
    else
    {
        glEnable(GL_CULL_FACE);
        switch (mode)
        {
        case CullMode::Front:          glCullFace(GL_FRONT); break;
        case CullMode::Back:           glCullFace(GL_BACK); break;
        case CullMode::FrontAndBack:   glCullFace(GL_FRONT_AND_BACK); break;
        }
    }
    m_StateCache.Cull = mode;
}

void OpenGLRendererAPI::SetBlendMode(bool enabled)
{
    if (m_StateCache.Blend == enabled) return;

    if (enabled) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    
    m_StateCache.Blend = enabled;
}

void OpenGLRendererAPI::SetBlendFunc(BlendFactor src, BlendFactor dst)
{
    if (m_StateCache.SrcBlend == src && m_StateCache.DstBlend == dst) return;

    auto toGL = [](BlendFactor factor) -> GLenum {
        switch (factor)
        {
        case BlendFactor::Zero:                return GL_ZERO;
        case BlendFactor::One:                 return GL_ONE;
        case BlendFactor::SrcColor:            return GL_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:    return GL_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor:            return GL_DST_COLOR;
        case BlendFactor::OneMinusDstColor:    return GL_ONE_MINUS_DST_COLOR;
        case BlendFactor::SrcAlpha:            return GL_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:    return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstAlpha:            return GL_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:    return GL_ONE_MINUS_DST_ALPHA;
        }
        return GL_ONE;
    };
    glBlendFunc(toGL(src), toGL(dst));
    m_StateCache.SrcBlend = src;
    m_StateCache.DstBlend = dst;
}

void OpenGLRendererAPI::SetLineWidth(float width)
{
    glLineWidth(width);
}

void OpenGLRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
    vertexArray->Bind();
    uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
    glDrawElements(GL_TRIANGLES, (GLsizei)count, GL_UNSIGNED_INT, nullptr);
}

void OpenGLRendererAPI::DrawIndexedLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
    vertexArray->Bind();
    uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
    glDrawElements(GL_LINES, (GLsizei)count, GL_UNSIGNED_INT, nullptr);
}

void OpenGLRendererAPI::DrawLines(const std::shared_ptr<VertexArray>& vertexArray, uint32_t vertexCount)
{
    vertexArray->Bind();
    glDrawArrays(GL_LINES, 0, (GLsizei)vertexCount);
}

void OpenGLRendererAPI::SetPolygonMode(PolygonMode mode)
{
    GLenum glMode = GL_FILL;
    switch (mode)
    {
    case PolygonMode::Fill:
        glMode = GL_FILL;
        break;
    case PolygonMode::Line:
        glMode = GL_LINE;
        break;
    case PolygonMode::Point:
        glMode = GL_POINT;
        break;
    }
    glPolygonMode(GL_FRONT_AND_BACK, glMode);
}

void OpenGLRendererAPI::SetPolygonOffset(bool enabled, float factor, float units)
{
    if (enabled)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glEnable(GL_POLYGON_OFFSET_LINE);
        glEnable(GL_POLYGON_OFFSET_POINT);
        glPolygonOffset(factor, units);
    }
    else
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
        glDisable(GL_POLYGON_OFFSET_LINE);
        glDisable(GL_POLYGON_OFFSET_POINT);
    }
}

void OpenGLRendererAPI::DrawArrays(uint32_t vertexCount)
{
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertexCount);
}

void OpenGLRendererAPI::DrawIndexedInstanced(const std::shared_ptr<VertexArray>& vertexArray, uint32_t instanceCount,
                                             uint32_t indexCount)
{
    uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
    glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)count, GL_UNSIGNED_INT, nullptr, (GLsizei)instanceCount);
}

void OpenGLRendererAPI::DrawArraysInstanced(uint32_t vertexCount, uint32_t instanceCount)
{
    glDrawArraysInstanced(GL_TRIANGLES, 0, (GLsizei)vertexCount, (GLsizei)instanceCount);
}

void OpenGLRendererAPI::SetTexture(uint32_t slot, uint32_t textureId, bool isCubemap)
{
    if (m_StateCache.BoundTextures.count(slot) && m_StateCache.BoundTextures[slot] == textureId)
        return;

    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, textureId);
    m_StateCache.BoundTextures[slot] = textureId;
}

bool OpenGLRendererAPI::IsDepthTestEnabled() const
{
    return glIsEnabled(GL_DEPTH_TEST);
}

bool OpenGLRendererAPI::IsBlendEnabled() const
{
    return glIsEnabled(GL_BLEND);
}

bool OpenGLRendererAPI::IsCullFaceEnabled() const
{
    return glIsEnabled(GL_CULL_FACE);
}

} // namespace CHEngine
