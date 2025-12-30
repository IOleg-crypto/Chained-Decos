#ifndef SERVICE_REGISTRY_H
#define SERVICE_REGISTRY_H

#include "core/Log.h"
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace CHEngine
{

class ServiceRegistry
{
public:
    template <typename T> static void Register(std::shared_ptr<T> service)
    {
        s_Services[std::type_index(typeid(T))] = service;
        CD_CORE_TRACE("Service registered: %s", typeid(T).name());
    }

    template <typename T> static std::shared_ptr<T> Get()
    {
        auto it = s_Services.find(std::type_index(typeid(T)));
        if (it != s_Services.end())
        {
            return std::static_pointer_cast<T>(it->second);
        }
        CD_CORE_ERROR("Service not found: %s", typeid(T).name());
        return nullptr;
    }

    static void Clear()
    {
        s_Services.clear();
        CD_CORE_TRACE("ServiceRegistry cleared");
    }

private:
    inline static std::unordered_map<std::type_index, std::shared_ptr<void>> s_Services;
};

} // namespace CHEngine

#endif // SERVICE_REGISTRY_H
