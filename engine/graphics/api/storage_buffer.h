#ifndef CH_STORAGE_BUFFER_H
#define CH_STORAGE_BUFFER_H

#include <memory>
#include <cstdint>

namespace Chained
{
    class StorageBuffer
    {
    public:
        virtual ~StorageBuffer() = default;

        virtual void BindBase(uint32_t slot) const = 0;
        virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

        static std::shared_ptr<StorageBuffer> Create(uint32_t size);
    };
}

#endif // CH_STORAGE_BUFFER_H
