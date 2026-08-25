#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/graphics_device.h"
#include "opengl/gl_buffer.h"
#include "opengl/gl_uniform_buffer.h"

namespace Chained
{

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (GraphicsDevice::GetAPI())
		{
		case GraphicsDevice::API::None:
			return nullptr;
		case GraphicsDevice::API::OpenGL:
			return std::make_shared<GLVertexBuffer>(size);
		}
		return nullptr;
	}

	std::shared_ptr<VertexBuffer> VertexBuffer::Create(const float* vertices, uint32_t size)
	{
		switch (GraphicsDevice::GetAPI())
		{
		case GraphicsDevice::API::None:
			return nullptr;
		case GraphicsDevice::API::OpenGL:
			return std::make_shared<GLVertexBuffer>(vertices, size);
		}
		return nullptr;
	}

	std::shared_ptr<IndexBuffer> IndexBuffer::Create(const uint32_t* indices, uint32_t count)
	{
		switch (GraphicsDevice::GetAPI())
		{
		case GraphicsDevice::API::None:
			return nullptr;
		case GraphicsDevice::API::OpenGL:
			return std::make_shared<GLIndexBuffer>(indices, count);
		}
		return nullptr;
	}

	std::shared_ptr<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (GraphicsDevice::GetAPI())
		{
		case GraphicsDevice::API::None:
			return nullptr;
		case GraphicsDevice::API::OpenGL:
			return std::make_shared<GLUniformBuffer>(size, binding);
		}
		return nullptr;
	}

} // namespace Chained
