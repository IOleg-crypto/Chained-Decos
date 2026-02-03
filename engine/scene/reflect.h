#ifndef CH_REFLECT_H
#define CH_REFLECT_H

#include "entt/entt.hpp"
#include <string>
#include <vector>
#include <cstddef>

namespace CHEngine
{
    // Property metadata structure
    struct PropertyInfo
    {
        const char* Name;
        size_t Offset;
        const char* TypeName;
    };

    // Base trait template
    template<typename T>
    struct ReflectData
    {
        static constexpr bool Registered = false;
        static const std::vector<PropertyInfo>& GetProperties() 
        { 
            static std::vector<PropertyInfo> empty; 
            return empty; 
        }
    };
} // namespace CHEngine

// Macros to be used INSIDE namespace CHEngine
#define BEGIN_REFLECT(type) \
    template<> struct ReflectData<type> { \
        using Self = type; \
        static constexpr bool Registered = true; \
        static const std::vector<PropertyInfo>& GetProperties() { \
            static std::vector<PropertyInfo> props = {

#define PROPERTY(type, name, label) { label, offsetof(Self, name), #type },

#define END_REFLECT() \
            }; return props; \
        } \
    };

#endif // CH_REFLECT_H
