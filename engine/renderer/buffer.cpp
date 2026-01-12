#include "buffer.h"
#include "engine/core/log.h"
#include "engine/core/types.h"
#include "opengl_buffer.h"
#include "render_api.h"

namespace CHEngine
{

std::shared_ptr<VertexBuffer> VertexBuffer::Create(uint32_t size)
{
    switch (RenderAPI::GetAPI())
    {
    case RenderAPI::API::None:
        CH_CORE_ASSERT(false, "RenderAPI::None is currently not supported!");
        return nullptr;
    case RenderAPI::API::OpenGL:
        return std::make_shared<OpenGLVertexBuffer>(size);
    }

    CH_CORE_ASSERT(false, "Unknown RenderAPI!");
    return nullptr;
}

std::shared_ptr<VertexBuffer> VertexBuffer::Create(float *vertices, uint32_t size)
{
    switch (RenderAPI::GetAPI())
    {
    case RenderAPI::API::None:
        CH_CORE_ASSERT(false, "RenderAPI::None is currently not supported!");
        return nullptr;
    case RenderAPI::API::OpenGL:
        return std::make_shared<OpenGLVertexBuffer>(vertices, size);
    }

    CH_CORE_ASSERT(false, "Unknown RenderAPI!");
    return nullptr;
}

std::shared_ptr<IndexBuffer> IndexBuffer::Create(uint32_t *indices, uint32_t count)
{
    switch (RenderAPI::GetAPI())
    {
    case RenderAPI::API::None:
        CH_CORE_ASSERT(false, "RenderAPI::None is currently not supported!");
        return nullptr;
    case RenderAPI::API::OpenGL:
        return std::make_shared<OpenGLIndexBuffer>(indices, count);
    }

    CH_CORE_ASSERT(false, "Unknown RenderAPI!");
    return nullptr;
}

} // namespace CHEngine
