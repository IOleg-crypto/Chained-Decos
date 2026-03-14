#include "raylib_renderer_api.h"
#include "raylib.h"
#include "rlgl.h"
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
    ::ClearBackground(m_ClearColor);
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
