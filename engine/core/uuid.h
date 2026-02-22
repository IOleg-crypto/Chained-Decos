#ifndef CH_UUID_H
#define CH_UUID_H

#include <cstddef>
#include <cstdint>
#include <functional>

namespace CHEngine
{
class UUID
{
public:
    UUID();
    UUID(uint64_t uuid);
    UUID(const std::string& uuidStr);
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
    std::size_t operator()(const CHEngine::UUID& uuid) const
    {
        return std::hash<uint64_t>()((uint64_t)uuid);
    }
};
} // namespace std

#endif // CH_UUID_H
