#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/renderer_api.h"
#include "opengl/opengl_buffer.h"
#include "opengl/opengl_uniform_buffer.h"

namespace CHEngine
{

std::shared_ptr<VertexBuffer> VertexBuffer::Create(uint32_t size)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(size);
    }
    return nullptr;
}

std::shared_ptr<VertexBuffer> VertexBuffer::Create(const float* vertices, uint32_t size)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLVertexBuffer>(vertices, size);
    }
    return nullptr;
}

std::shared_ptr<IndexBuffer> IndexBuffer::Create(const uint32_t* indices, uint32_t count)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLIndexBuffer>(indices, count);
    }
    return nullptr;
}

std::shared_ptr<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLUniformBuffer>(size, binding);
    }
    return nullptr;
}

} // namespace CHEngine
