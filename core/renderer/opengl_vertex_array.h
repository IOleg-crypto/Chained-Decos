#ifndef CD_CORE_RENDERER_OPENGL_VERTEX_ARRAY_H
#define CD_CORE_RENDERER_OPENGL_VERTEX_ARRAY_H

#include "vertex_array.h"

namespace CHEngine
{

class OpenGLVertexArray : public VertexArray
{
public:
    OpenGLVertexArray();
    virtual ~OpenGLVertexArray();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void AddVertexBuffer(const std::shared_ptr<VertexBuffer> &vertexBuffer) override;
    virtual void SetIndexBuffer(const std::shared_ptr<IndexBuffer> &indexBuffer) override;

    virtual const std::vector<std::shared_ptr<VertexBuffer>> &GetVertexBuffers() const override
    {
        return m_VertexBuffers;
    }
    virtual const std::shared_ptr<IndexBuffer> &GetIndexBuffer() const override
    {
        return m_IndexBuffer;
    }

private:
    uint32_t m_RendererID;
    uint32_t m_VertexBufferIndex = 0;
    std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
    std::shared_ptr<IndexBuffer> m_IndexBuffer;
};

} // namespace CHEngine

#endif // CD_CORE_RENDERER_OPENGL_VERTEX_ARRAY_H
