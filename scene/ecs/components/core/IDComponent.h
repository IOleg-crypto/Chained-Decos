#pragma once

#include <cstdint>
#include <random>

namespace CHEngine
{

/**
 * IDComponent - Unique identifier for entities
 *
 * Each entity gets a unique ID (UUID) for serialization and referencing.
 */
struct IDComponent
{
    uint64_t ID;

    IDComponent() : ID(GenerateUUID())
    {
    }
    IDComponent(uint64_t id) : ID(id == 0 ? GenerateUUID() : id)
    {
    }

    operator uint64_t() const
    {
        return ID;
    }

private:
    static uint64_t GenerateUUID()
    {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis;
        return dis(gen);
    }
};

} // namespace CHEngine
