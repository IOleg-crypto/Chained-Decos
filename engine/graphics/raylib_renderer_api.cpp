#include "raylib_renderer_api.h"
#include "raylib.h"
#include "rlgl.h"
#include "external/glad.h"
#include "vertex_array.h"

namespace CHEngine
{

void RaylibRendererAPI::Init()
{
    // Raylib's initialization is usually handled during window creation,
    // but we can put any RLGL-specific global state setup here.
}

void RaylibRendererAPI::SetViewport(int x, int y, int width, int height)
{
    rlViewport(x, y, width, height);
}

void RaylibRendererAPI::SetClearColor(const Color& color)
{
    m_ClearColor = color;
}

void RaylibRendererAPI::Clear()
{
    rlClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, m_ClearColor.a);
    rlClearScreenBuffers();
}
 
void RaylibRendererAPI::SetDepthFunc(DepthFunc func)
{
    // Use raw hex values equivalent to GL_LESS, GL_LEQUAL etc
    // since we can't reliably include internal Raylib/GL headers here.
    switch (func)
    {
        case DepthFunc::Never:    glDepthFunc(0x0200); break;
        case DepthFunc::Less:     glDepthFunc(0x0201); break;
        case DepthFunc::Equal:    glDepthFunc(0x0202); break;
        case DepthFunc::LEqual:   glDepthFunc(0x0203); break;
        case DepthFunc::Greater:  glDepthFunc(0x0204); break;
        case DepthFunc::NotEqual: glDepthFunc(0x0205); break;
        case DepthFunc::GEqual:   glDepthFunc(0x0206); break;
        case DepthFunc::Always:   glDepthFunc(0x0207); break;
    }
}

void RaylibRendererAPI::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray, uint32_t indexCount)
{
    vertexArray->Bind();
    auto& indexBuffer = vertexArray->GetIndexBuffer();
    indexBuffer->Bind();
    
    uint32_t count = indexCount ? indexCount : indexBuffer->GetCount();
    
    // In RLGL, we usually use rlDrawVertexArrayElements. 
    // If an EBO is bound, the last parameter should be 0.
    rlDrawVertexArrayElements(0, count, 0);
    
    indexBuffer->Unbind();
    vertexArray->Unbind();
}

} // namespace CHEngine
