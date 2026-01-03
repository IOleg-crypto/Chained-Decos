#ifndef CD_CORE_RENDERER_VERTEX_ARRAY_H
#define CD_CORE_RENDERER_VERTEX_ARRAY_H

#include "buffer.h"
#include <memory>
#include <vector>

namespace CHEngine
{

class VertexArray
{
public:
    virtual ~VertexArray() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual void AddVertexBuffer(const std::shared_ptr<VertexBuffer> &vertexBuffer) = 0;
    virtual void SetIndexBuffer(const std::shared_ptr<IndexBuffer> &indexBuffer) = 0;

    virtual const std::vector<std::shared_ptr<VertexBuffer>> &GetVertexBuffers() const = 0;
    virtual const std::shared_ptr<IndexBuffer> &GetIndexBuffer() const = 0;

    static std::shared_ptr<VertexArray> Create();
};

} // namespace CHEngine

#endif // CD_CORE_RENDERER_VERTEX_ARRAY_H
