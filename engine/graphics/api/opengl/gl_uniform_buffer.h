#ifndef CH_OPENGL_UNIFORM_BUFFER_H
#define CH_OPENGL_UNIFORM_BUFFER_H

#include "engine/graphics/api/buffer.h"

namespace Chained
{
	class GLUniformBuffer : public UniformBuffer
	{
	public:
		GLUniformBuffer(uint32_t size, uint32_t binding);
		virtual ~GLUniformBuffer();

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void BindBase(uint32_t binding) override;

	private:
		uint32_t m_RendererID = 0;
	};
} // namespace Chained

#endif // CH_OPENGL_UNIFORM_BUFFER_H
