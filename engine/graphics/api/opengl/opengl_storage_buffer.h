#ifndef CH_OPENGL_STORAGE_BUFFER_H
#define CH_OPENGL_STORAGE_BUFFER_H

#include "engine/graphics/api/storage_buffer.h"

namespace Chained
{
    class OpenGLStorageBuffer : public StorageBuffer
    {
    public:
        OpenGLStorageBuffer(uint32_t size);
        virtual ~OpenGLStorageBuffer();

        virtual void BindBase(uint32_t slot) const override;
        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

    private:
        uint32_t m_RendererID = 0;
    };
}

#endif // CH_OPENGL_STORAGE_BUFFER_H
