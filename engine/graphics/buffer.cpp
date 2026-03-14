#include "buffer.h"
#include "renderer_api.h"
#include "raylib_buffer.h"

namespace CHEngine
{

std::shared_ptr<VertexBuffer> VertexBuffer::Create(uint32_t size)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::Raylib:  return std::make_shared<RaylibVertexBuffer>(size);
    }
    return nullptr;
}

std::shared_ptr<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::Raylib:  return std::make_shared<RaylibVertexBuffer>(vertices, size);
    }
    return nullptr;
}

std::shared_ptr<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::Raylib:  return std::make_shared<RaylibIndexBuffer>(indices, count);
    }
    return nullptr;
}

} // namespace CHEngine
