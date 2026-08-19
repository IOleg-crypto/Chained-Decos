#include "engine/graphics/api/storage_buffer.h"
#include "engine/graphics/api/graphics_device.h"
#include "opengl/gl_storage_buffer.h"

namespace Chained
{
	std::shared_ptr<StorageBuffer> StorageBuffer::Create(uint32_t size)
	{
		switch (GraphicsDevice::GetAPI())
		{
		case GraphicsDevice::API::None:
			return nullptr;
		case GraphicsDevice::API::OpenGL:
			return std::make_shared<GLStorageBuffer>(size);
		default:
			return nullptr;
		}
	}
} // namespace Chained
