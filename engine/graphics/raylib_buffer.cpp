#include "raylib_buffer.h"
#include "rlgl.h"

namespace CHEngine
{

// VertexBuffer ///////////////////////////////////////////////////////////

RaylibVertexBuffer::RaylibVertexBuffer(uint32_t size)
{
    m_RendererID = rlLoadVertexBuffer(nullptr, size, true);
}

RaylibVertexBuffer::RaylibVertexBuffer(float* vertices, uint32_t size)
{
    m_RendererID = rlLoadVertexBuffer(vertices, size, false);
}

RaylibVertexBuffer::~RaylibVertexBuffer()
{
    rlUnloadVertexBuffer(m_RendererID);
}

void RaylibVertexBuffer::Bind() const
{
    // RLGL handles binding internally or via vertex array state
}

void RaylibVertexBuffer::Unbind() const
{
}

void RaylibVertexBuffer::SetData(const void* data, uint32_t size)
{
    rlUpdateVertexBuffer(m_RendererID, data, size, 0);
}

// IndexBuffer ////////////////////////////////////////////////////////////

RaylibIndexBuffer::RaylibIndexBuffer(uint32_t* indices, uint32_t count)
    : m_Count(count)
{
    m_RendererID = rlLoadVertexBufferElement(indices, count * sizeof(uint32_t), false);
}

RaylibIndexBuffer::~RaylibIndexBuffer()
{
    rlUnloadVertexBuffer(m_RendererID);
}

void RaylibIndexBuffer::Bind() const
{
}

void RaylibIndexBuffer::Unbind() const
{
}

} // namespace CHEngine
