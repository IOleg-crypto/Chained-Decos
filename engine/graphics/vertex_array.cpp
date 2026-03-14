#include "vertex_array.h"
#include "renderer_api.h"
#include "raylib_vertex_array.h"

namespace CHEngine
{

std::shared_ptr<VertexArray> VertexArray::Create()
{
    switch (RendererAPI::GetAPI())
    {
        case RendererAPI::API::None:    return nullptr;
        case RendererAPI::API::Raylib:  return std::make_shared<RaylibVertexArray>();
    }
    return nullptr;
}

} // namespace CHEngine
