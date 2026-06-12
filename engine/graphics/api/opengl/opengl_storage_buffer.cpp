#include "opengl_storage_buffer.h"
#include <glad/gl.h>

namespace Chained
{
    OpenGLStorageBuffer::OpenGLStorageBuffer(uint32_t size)
    {
        glGenBuffers(1, &m_RendererID);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    OpenGLStorageBuffer::~OpenGLStorageBuffer()
    {
        glDeleteBuffers(1, &m_RendererID);
    }

    void OpenGLStorageBuffer::BindBase(uint32_t slot) const
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, m_RendererID);
    }

    void OpenGLStorageBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}
