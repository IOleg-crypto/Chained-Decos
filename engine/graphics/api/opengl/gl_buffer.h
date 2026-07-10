#ifndef CH_OPENGL_BUFFER_H
#define CH_OPENGL_BUFFER_H

#include "engine/graphics/api/buffer.h"

namespace Chained
{

class GLVertexBuffer : public VertexBuffer
{
public:
    GLVertexBuffer(uint32_t size);
    GLVertexBuffer(const float* vertices, uint32_t size);
    virtual ~GLVertexBuffer();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void SetData(const void* data, uint32_t size) override;

    virtual const BufferLayout& GetLayout() const override { return m_Layout; }
    virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

private:
    uint32_t m_RendererID = 0;
    BufferLayout m_Layout;
};

class GLIndexBuffer : public IndexBuffer
{
public:
    GLIndexBuffer(const uint32_t* indices, uint32_t count);
    virtual ~GLIndexBuffer();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual uint32_t GetCount() const override { return m_Count; }

private:
    uint32_t m_RendererID = 0;
    uint32_t m_Count;
};

} // namespace Chained

#endif // CH_OPENGL_BUFFER_H
