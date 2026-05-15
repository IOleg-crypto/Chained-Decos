#ifndef CH_UUID_H
#define CH_UUID_H

#include "engine/core/base.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace CHEngine
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
} // namespace CHEngine

namespace std
{
template <> struct hash<CHEngine::UUID>
{
    std::size_t operator()(const CHEngine::UUID& uuid) const noexcept {
        return std::hash<uint64_t>()((uint64_t)uuid);
    }
};
} // namespace std

#endif // CH_UUID_H
