#ifndef CH_RENDER_COMMAND_QUEUE_H
#define CH_RENDER_COMMAND_QUEUE_H

#include "engine/core/base.h"
#include "engine/core/log.h"
#include <cstdint>
#include <cstring>

namespace CHEngine
{
    class RenderCommandQueue
    {
    public:
        typedef void(*RenderCommandFn)(void*);

        RenderCommandQueue()
        {
            m_CommandBufferSize = 10 * 1024 * 1024; // 10 MB buffer
            m_CommandBuffer = new uint8_t[m_CommandBufferSize];
            m_CommandBufferPtr = m_CommandBuffer;
            memset(m_CommandBuffer, 0, m_CommandBufferSize);
        }

        ~RenderCommandQueue()
        {
            delete[] m_CommandBuffer;
        }

        void* Allocate(RenderCommandFn func, uint32_t size)
        {
            // Simple validation to ensure we don't overrun
            // In a production engine, this would wrap around or resize, 
            // but for now we assert to catch heavy usage.
            CH_CORE_ASSERT((m_CommandBufferPtr - m_CommandBuffer) + size + sizeof(RenderCommandFn) + sizeof(uint32_t) < m_CommandBufferSize, "Render Command Queue Overflow!");

            // 1. Store the function pointer
            *(RenderCommandFn*)m_CommandBufferPtr = func;
            m_CommandBufferPtr += sizeof(RenderCommandFn);

            // 2. Store the size of the command data
            *(uint32_t*)m_CommandBufferPtr = size;
            m_CommandBufferPtr += sizeof(uint32_t);

            // 3. Return the pointer to the memory where data should be written
            void* memory = m_CommandBufferPtr;
            m_CommandBufferPtr += size;
            
            return memory;
        }

        void Execute()
        {
            uint8_t* ptr = m_CommandBuffer;

            while (ptr < m_CommandBufferPtr)
            {
                RenderCommandFn function = *(RenderCommandFn*)ptr;
                ptr += sizeof(RenderCommandFn);

                uint32_t size = *(uint32_t*)ptr;
                ptr += sizeof(uint32_t);

                function(ptr);
                ptr += size;
            }

            m_CommandBufferPtr = m_CommandBuffer;
        }

    private:
        uint8_t* m_CommandBuffer;
        uint8_t* m_CommandBufferPtr;
        uint32_t m_CommandBufferSize;
    };
}

#endif // CH_RENDER_COMMAND_QUEUE_H
