#include "OpenGLVertexArray.h"
#include "OpenGLBuffer.h"
#include <glad.h>

namespace CHEngine
{

static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
{
    switch (type)
    {
    case ShaderDataType::Float:
        return GL_FLOAT;
    case ShaderDataType::Float2:
        return GL_FLOAT;
    case ShaderDataType::Float3:
        return GL_FLOAT;
    case ShaderDataType::Float4:
        return GL_FLOAT;
    case ShaderDataType::Mat3:
        return GL_FLOAT;
    case ShaderDataType::Mat4:
        return GL_FLOAT;
    case ShaderDataType::Int:
        return GL_INT;
    case ShaderDataType::Int2:
        return GL_INT;
    case ShaderDataType::Int3:
        return GL_INT;
    case ShaderDataType::Int4:
        return GL_INT;
    case ShaderDataType::Bool:
        return GL_BOOL;
    default:
        return 0;
    }
}

OpenGLVertexArray::OpenGLVertexArray()
{
    glCreateVertexArrays(1, &m_RendererID);
}

OpenGLVertexArray::~OpenGLVertexArray()
{
    glDeleteVertexArrays(1, &m_RendererID);
}

void OpenGLVertexArray::Bind() const
{
    glBindVertexArray(m_RendererID);
}

void OpenGLVertexArray::Unbind() const
{
    glBindVertexArray(0);
}

void OpenGLVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer> &vertexBuffer)
{
    const auto &layout = vertexBuffer->GetLayout();
    for (const auto &element : layout)
    {
        switch (element.Type)
        {
        case ShaderDataType::Float:
        case ShaderDataType::Float2:
        case ShaderDataType::Float3:
        case ShaderDataType::Float4:
        {
            glEnableVertexArrayAttrib(m_RendererID, m_VertexBufferIndex);
            glVertexArrayAttribFormat(m_RendererID, m_VertexBufferIndex,
                                      element.GetComponentCount(),
                                      ShaderDataTypeToOpenGLBaseType(element.Type),
                                      element.Normalized ? GL_TRUE : GL_FALSE, element.Offset);
            glVertexArrayAttribBinding(m_RendererID, m_VertexBufferIndex, 0);
            m_VertexBufferIndex++;
            break;
        }
        case ShaderDataType::Int:
        case ShaderDataType::Int2:
        case ShaderDataType::Int3:
        case ShaderDataType::Int4:
        case ShaderDataType::Bool:
        {
            glEnableVertexArrayAttrib(m_RendererID, m_VertexBufferIndex);
            glVertexArrayAttribIFormat(
                m_RendererID, m_VertexBufferIndex, element.GetComponentCount(),
                ShaderDataTypeToOpenGLBaseType(element.Type), element.Offset);
            glVertexArrayAttribBinding(m_RendererID, m_VertexBufferIndex, 0);
            m_VertexBufferIndex++;
            break;
        }
        case ShaderDataType::Mat3:
        case ShaderDataType::Mat4:
        {
            uint8_t count = element.GetComponentCount();
            for (uint8_t i = 0; i < count; i++)
            {
                glEnableVertexArrayAttrib(m_RendererID, m_VertexBufferIndex);
                glVertexArrayAttribFormat(m_RendererID, m_VertexBufferIndex, count,
                                          ShaderDataTypeToOpenGLBaseType(element.Type),
                                          element.Normalized ? GL_TRUE : GL_FALSE,
                                          element.Offset + sizeof(float) * count * i);
                glVertexArrayAttribBinding(m_RendererID, m_VertexBufferIndex, 0);
                glVertexArrayBindingDivisor(m_RendererID, m_VertexBufferIndex, 1);
                m_VertexBufferIndex++;
            }
            break;
        }
        default:
            break;
        }
    }

    m_VertexBuffers.push_back(vertexBuffer);
}

void OpenGLVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer> &indexBuffer)
{
    glVertexArrayElementBuffer(
        m_RendererID, std::static_pointer_cast<OpenGLIndexBuffer>(indexBuffer)->m_RendererID);
    m_IndexBuffer = indexBuffer;
}

} // namespace CHEngine
