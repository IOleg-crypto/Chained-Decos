#ifndef CH_OPENGL_BUFFER_H
#define CH_OPENGL_BUFFER_H

#include "buffer.h"

namespace CHEngine
{
class OpenGLVertexBuffer : public VertexBuffer
{
public:
    OpenGLVertexBuffer(uint32_t size);
    OpenGLVertexBuffer(float *vertices, uint32_t size);
    virtual ~OpenGLVertexBuffer();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void SetData(const void *data, uint32_t size) override;

    virtual const BufferLayout &GetLayout() const override
    {
        return m_Layout;
    }
    virtual void SetLayout(const BufferLayout &layout) override
    {
        m_Layout = layout;
    }

private:
    uint32_t m_RenderID;
    BufferLayout m_Layout;
};

class OpenGLIndexBuffer : public IndexBuffer
{
public:
    OpenGLIndexBuffer(uint32_t *indices, uint32_t count);
    virtual ~OpenGLIndexBuffer();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual uint32_t GetCount() const override
    {
        return m_Count;
    }

    // Make sure m_RendererID is accessible for SetIndexBuffer
public:
    uint32_t m_RenderID;
    uint32_t m_Count;
};

} // namespace CHEngine

#endif // CH_OPENGL_BUFFER_H
