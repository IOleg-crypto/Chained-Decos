#include "raylib_vertex_array.h"
#include "rlgl.h"

namespace CHEngine
{

static uint32_t ShaderDataTypeToRaylibBaseType(ShaderDataType type)
{
    switch (type)
    {
        case ShaderDataType::Float:    return 0x1406; // RL_FLOAT
        case ShaderDataType::Float2:   return 0x1406; // RL_FLOAT
        case ShaderDataType::Float3:   return 0x1406; // RL_FLOAT
        case ShaderDataType::Float4:   return 0x1406; // RL_FLOAT
        case ShaderDataType::Mat3:     return 0x1406; // RL_FLOAT
        case ShaderDataType::Mat4:     return 0x1406; // RL_FLOAT
        case ShaderDataType::Int:      return 0x1404; // GL_INT
        case ShaderDataType::Int2:     return 0x1404;
        case ShaderDataType::Int3:     return 0x1404;
        case ShaderDataType::Int4:     return 0x1404;
        case ShaderDataType::Bool:     return 0x1401; // RL_UNSIGNED_BYTE
    }

    CH_CORE_ASSERT(false, "Unknown ShaderDataType!");
    return 0;
}

RaylibVertexArray::RaylibVertexArray()
{
    m_RendererID = rlLoadVertexArray();
}

RaylibVertexArray::~RaylibVertexArray()
{
    rlUnloadVertexArray(m_RendererID);
}

void RaylibVertexArray::Bind() const
{
    rlEnableVertexArray(m_RendererID);
}

void RaylibVertexArray::Unbind() const
{
    rlDisableVertexArray();
}

void RaylibVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
{
    CH_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

    rlEnableVertexArray(m_RendererID);
    vertexBuffer->Bind();

    uint32_t index = 0;
    const auto& layout = vertexBuffer->GetLayout();
    for (const auto& element : layout)
    {
        rlEnableVertexAttribute(index);
        rlSetVertexAttribute(index, 
            element.GetComponentCount(), 
            ShaderDataTypeToRaylibBaseType(element.Type), 
            element.Normalized ? true : false, 
            layout.GetStride(), 
            (int)element.Offset);
        index++;
    }

    m_VertexBuffers.push_back(vertexBuffer);
}

void RaylibVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
{
    rlEnableVertexArray(m_RendererID);
    indexBuffer->Bind();

    m_IndexBuffer = indexBuffer;
}

} // namespace CHEngine
