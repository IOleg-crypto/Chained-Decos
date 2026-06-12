#ifndef CH_OPENGL_UNIFORM_BUFFER_H
#define CH_OPENGL_UNIFORM_BUFFER_H

#include "engine/graphics/api/buffer.h"

namespace Chained
{
class OpenGLUniformBuffer : public UniformBuffer
{
public:
    OpenGLUniformBuffer(uint32_t size, uint32_t binding);
    virtual ~OpenGLUniformBuffer();

    virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

private:
    uint32_t m_RendererID = 0;
};
} // namespace Chained

#endif // CH_OPENGL_UNIFORM_BUFFER_H
