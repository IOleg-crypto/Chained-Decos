#ifndef CH_RAYLIB_BUFFER_H
#define CH_RAYLIB_BUFFER_H

#include "buffer.h"

namespace CHEngine
{

class RaylibVertexBuffer : public VertexBuffer
{
public:
    RaylibVertexBuffer(uint32_t size);
    RaylibVertexBuffer(float* vertices, uint32_t size);
    virtual ~RaylibVertexBuffer();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void SetData(const void* data, uint32_t size) override;

    virtual const BufferLayout& GetLayout() const override { return m_Layout; }
    virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }

private:
    uint32_t m_RendererID;
    BufferLayout m_Layout;
};

class RaylibIndexBuffer : public IndexBuffer
{
public:
    RaylibIndexBuffer(uint32_t* indices, uint32_t count);
    virtual ~RaylibIndexBuffer();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual uint32_t GetCount() const override { return m_Count; }

private:
    uint32_t m_RendererID;
    uint32_t m_Count;
};

} // namespace CHEngine

#endif // CH_RAYLIB_BUFFER_H
