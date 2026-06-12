#include "uuid.h"
#include <random>

namespace Chained
{
static thread_local std::random_device s_RandomDevice;
static thread_local std::mt19937_64 s_Engine(s_RandomDevice());
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
    auto [ptr, ec] = std::from_chars(uuidStr.data(), uuidStr.data() + uuidStr.size(), m_UUID);

    // Якщо парсинг завершився помилкою (наприклад, рядок порожній або там текст)
    if (ec != std::errc{})
    {
        m_UUID = 0; // Гарантовано і безпечно зануляємо хендл (Invalid ID)
    }
}


std::string UUID::ToString() const
{
    return std::to_string(m_UUID);
}
} // namespace Chained
