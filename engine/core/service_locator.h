#ifndef CH_SERVICE_LOCATOR_H
#define CH_SERVICE_LOCATOR_H

#include "engine/core/engine_module.h"
#include "engine/common/engine_assert.h"
#include "engine/common/base.h"
#include <memory>
#include <shared_mutex>
#include <type_traits> 
#include <typeindex>
#include <unordered_map>

namespace Chained
{
class CH_API ServiceLocator
{
public:
    // Registers a service by its type. Takes ownership of the service pointer.
    // T must inherit from EngineModule.
    template <typename T> static void Provide(T* service)
    {
        static_assert(std::is_class_v<T>, "ServiceLocator: Provided type T must be a class or a struct!");
        static_assert(std::is_base_of_v<EngineModule, T>,
                      "ServiceLocator: Provided type T must inherit from EngineModule!");
        static_assert(std::has_virtual_destructor_v<EngineModule>,
                      "ServiceLocator: EngineModule must have a virtual destructor!");

        std::unique_lock<std::shared_mutex> lock(GetMutex());

        CH_CORE_ASSERT(!s_IsLocked, "ServiceLocator: Cannot provide a service after the locator has been locked!");
        if (s_IsLocked)
        {
            return;
        }

        auto& services = GetInternalMap();
        CH_CORE_ASSERT(services.find(typeid(T)) == services.end(), "ServiceLocator: Service already provided!");

        // shared_ptr<T> -> shared_ptr<void> preserves the correct deleter automatically
        auto sharedService = std::shared_ptr<T>(service);
        services[typeid(T)] = sharedService;
        GetModuleOrder().push_back(sharedService); // ← was GetSystemsOrder() (bug fix)
    }

    // Registers a NON-OWNING reference to an existing service.
    // Use this when the service lifetime is managed externally (e.g. existing singletons).
    // ServiceLocator::Shutdown() will NOT call delete on such services.
    template <typename T> static void ProvideRef(T* service)
    {
        static_assert(std::is_class_v<T>, "ServiceLocator: Provided type T must be a class or a struct!");
        static_assert(std::is_base_of_v<EngineModule, T>,
                      "ServiceLocator: Provided type T must inherit from EngineModule!");

        std::unique_lock<std::shared_mutex> lock(GetMutex());

        CH_CORE_ASSERT(!s_IsLocked, "ServiceLocator: Cannot provide a service after the locator has been locked!");
        if (s_IsLocked)
        {
            return;
        }

        auto& services = GetInternalMap();
        CH_CORE_ASSERT(services.find(typeid(T)) == services.end(), "ServiceLocator: Service already provided!");

        // No-op deleter: ServiceLocator does NOT own this pointer
        auto sharedService = std::shared_ptr<T>(service, [](T*) {});
        services[typeid(T)] = sharedService;
        // NOTE: we do NOT add to GetModuleOrder() — lifecycle is external
    }

    static void Lock()
    {
        std::unique_lock<std::shared_mutex> lock(GetMutex());
        s_IsLocked = true;
    }
    static void InitializeModule()
    {
        std::shared_lock<std::shared_mutex> lock(GetMutex());
        for (auto& module : GetModuleOrder())
        {
            if (!module->IsEnabled())
                continue;

            try
            {
                module->Initialize();
            }
            catch (const std::exception& e)
            {
                CH_CORE_ERROR("ServiceLocator: Module initialization failed with exception: {}", e.what());
                module->SetEnabled(false);
            }
            catch (...)
            {
                CH_CORE_ERROR("ServiceLocator: Module initialization failed with an unknown exception.");
                module->SetEnabled(false);
            }
        }
    }

    static void UpdateModules(Timestep ts)
    {
        std::shared_lock<std::shared_mutex> lock(GetMutex());
        for (auto& module : GetModuleOrder())
        {
            if (module->IsEnabled())
            {
                module->Update(ts);
            }
        }
    }

    static void Shutdown()
    {
        std::vector<std::shared_ptr<EngineModule>> modulesToShutdown;
        {
            std::unique_lock<std::shared_mutex> lock(GetMutex());
            s_IsLocked = false;
            modulesToShutdown = GetModuleOrder();
        }

        for (auto it = modulesToShutdown.rbegin(); it != modulesToShutdown.rend(); ++it)
        {
            if ((*it)->IsEnabled())
            {
                (*it)->Shutdown();
            }
        }

        {
            std::unique_lock<std::shared_mutex> lock(GetMutex());
            GetInternalMap().clear();
            GetModuleOrder().clear();
        }
    }
    static bool IsAvailable()
    {
        std::shared_lock<std::shared_mutex> lock(GetMutex());
        return !GetInternalMap().empty();
    }
    template <typename T> static T* Get()
    {
        static_assert(std::is_base_of_v<EngineModule, T>,
                      "ServiceLocator: Requested type T must inherit from EngineModule!");

        std::shared_lock<std::shared_mutex> lock(GetMutex());
        auto& services = GetInternalMap();
        auto it = services.find(typeid(T));

        if (it != services.end())
        {
            T* svc = static_cast<T*>(it->second.get());
            if (!svc->IsEnabled())
            {
                CH_CORE_WARN("ServiceLocator: Requested service '{}' is disabled!", typeid(T).name());
                // Still returning it as old code expects a valid pointer, but consumers should check IsEnabled() (or we can return nullptr, but returning nullptr crashes unprotected callers). 
                // Let's return it, but emit a warning.
            }
            return svc;
        }

        CH_CORE_ASSERT(false, "ServiceLocator: Requested service not found!");
        return nullptr;
    }

    template <typename T> static T* TryGet()
    {
        static_assert(std::is_base_of_v<EngineModule, T>,
                      "ServiceLocator: Requested type T must inherit from EngineModule!");

        std::shared_lock<std::shared_mutex> lock(GetMutex());
        auto& services = GetInternalMap();
        auto it = services.find(typeid(T));

        if (it != services.end())
        {
            T* svc = static_cast<T*>(it->second.get());
            if (svc && svc->IsEnabled())
                return svc;
        }
        return nullptr;
    }

    template <typename T> static bool Has()
    {
        static_assert(std::is_base_of_v<EngineModule, T>,
                      "ServiceLocator: Checked type T must inherit from EngineModule!");

        std::shared_lock<std::shared_mutex> lock(GetMutex());
        auto& services = GetInternalMap();
        return services.find(typeid(T)) != services.end();
    }

private:
    static std::unordered_map<std::type_index, std::shared_ptr<void>>& GetInternalMap()
    {
        static std::unordered_map<std::type_index, std::shared_ptr<void>> s_Services;
        return s_Services;
    }

    static std::vector<std::shared_ptr<EngineModule>>& GetModuleOrder()
    {
        static std::vector<std::shared_ptr<EngineModule>> s_Order;
        return s_Order;
    }

    static std::shared_mutex& GetMutex()
    {
        static std::shared_mutex s_Mutex;
        return s_Mutex;
    }

    inline static bool s_IsLocked = false;
};
} // namespace Chained

#endif // CH_SERVICE_LOCATOR_H