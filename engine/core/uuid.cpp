#include "uuid.h"
#include <random>

namespace CHEngine
{
static std::random_device s_RandomDevice;
static std::mt19937_64 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

UUID::UUID()
    : m_UUID(s_UniformDistribution(s_Engine))
{
}

UUID::UUID(uint64_t uuid)
    : m_UUID(uuid)
{
}

UUID::UUID(const std::string& uuidStr)
{
    try
    {
        m_UUID = std::stoull(uuidStr);
    }
    catch (...)
    {
        m_UUID = 0;
    }
}

std::string UUID::ToString() const
{
    return std::to_string(m_UUID);
}
} // namespace CHEngine
