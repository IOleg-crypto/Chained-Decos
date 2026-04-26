#include "engine/graphics/api/storage_buffer.h"
#include "engine/graphics/api/renderer_api.h"
#include "opengl/opengl_storage_buffer.h"

namespace CHEngine
{
    std::shared_ptr<StorageBuffer> StorageBuffer::Create(uint32_t size)
    {
        switch (RendererAPI::GetAPI())
        {
            case RendererAPI::API::None:    return nullptr;
            case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLStorageBuffer>(size);
        }
        return nullptr;
    }
}
