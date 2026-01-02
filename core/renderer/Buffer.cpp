#include "core/log.h"
#include "buffer.h"
#include "opengl_buffer.h"
#include "renderer_api.h"
#include "core/Base.h"

namespace CHEngine
{

std::shared_ptr<VertexBuffer> VertexBuffer::Create(uint32_t size)
{
    switch (RendererAPI::GetAPI())
    {
    case RendererAPI::API::None:
        CD_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
        return nullptr;
    case RendererAPI::API::OpenGL:
        return std::make_shared<OpenGLVertexBuffer>(size);
    }

    CD_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

std::shared_ptr<VertexBuffer> VertexBuffer::Create(float *vertices, uint32_t size)
{
    switch (RendererAPI::GetAPI())
    {
    case RendererAPI::API::None:
        CD_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
        return nullptr;
    case RendererAPI::API::OpenGL:
        return std::make_shared<OpenGLVertexBuffer>(vertices, size);
    }

    CD_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

std::shared_ptr<IndexBuffer> IndexBuffer::Create(uint32_t *indices, uint32_t count)
{
    switch (RendererAPI::GetAPI())
    {
    case RendererAPI::API::None:
        CD_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
        return nullptr;
    case RendererAPI::API::OpenGL:
        return std::make_shared<OpenGLIndexBuffer>(indices, count);
    }

    CD_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

} // namespace CHEngine
#include "core/log.h"

