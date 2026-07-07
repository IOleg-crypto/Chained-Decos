#ifndef CH_UUID_H
#define CH_UUID_H

#include "engine/common/base.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace Chained
{
class CH_API UUID
{
public:
    UUID();
    UUID(uint64_t uuid);
    explicit UUID(const std::string& uuidStr);
    UUID(const UUID&) = default;

    std::string ToString() const;

    operator uint64_t() const
    {
        return m_UUID;
    }

private:
    uint64_t m_UUID;
};
} // namespace Chained

namespace std
{
template <> struct hash<Chained::UUID>
{
    std::size_t operator()(const Chained::UUID& uuid) const noexcept {
        return std::hash<uint64_t>()((uint64_t)uuid);
    }
};
} // namespace std

#endif // CH_UUID_H
