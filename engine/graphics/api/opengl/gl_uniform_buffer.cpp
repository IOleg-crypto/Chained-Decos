#include "gl_uniform_buffer.h"
#include <glad/gl.h>

namespace Chained
{
GLUniformBuffer::GLUniformBuffer(uint32_t size, uint32_t binding)
{
    glGenBuffers(1, &m_RendererID);
    glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RendererID);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

GLUniformBuffer::~GLUniformBuffer()
{
    glDeleteBuffers(1, &m_RendererID);
}

void GLUniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
{
    glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
}
} // namespace Chained
