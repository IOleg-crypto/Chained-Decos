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
    // Registers a service by its type. Takes ownership of the service.
    // T must inherit from EngineModule.
    // On rejection (locked locator, duplicate registration) the service is destroyed
    // by the unique_ptr — no leak, unlike the old raw-pointer overload.
    template <typename T> static void Provide(std::unique_ptr<T> service)
    {
        static_assert(std::is_class_v<T>, "ServiceLocator: Provided type T must be a class or a struct!");
        static_assert(std::is_base_of_v<EngineModule, T>,
                      "ServiceLocator: Provided type T must inherit from EngineModule!");
        static_assert(std::has_virtual_destructor_v<EngineModule>,
                      "ServiceLocator: EngineModule must have a virtual destructor!");

        std::unique_lock<std::shared_mutex> lock(GetMutex());

        // Handled gracefully (log + destroy via unique_ptr) rather than asserted,
        // so release builds and tests behave the same as debug builds.
        if (s_IsLocked)
        {
            CH_CORE_ERROR("ServiceLocator: Provide<{}> rejected — locator is locked.", typeid(T).name());
            return;
        }

        auto& services = GetInternalMap();
        if (services.find(typeid(T)) != services.end())
        {
            // Bail explicitly so the duplicate doesn't overwrite the map entry
            // while the old instance lingers in ModuleOrder.
            CH_CORE_ERROR("ServiceLocator: Provide<{}> rejected — service already provided.", typeid(T).name());
            return;
        }

        // shared_ptr<T> -> shared_ptr<void> preserves the correct deleter automatically
        std::shared_ptr<T> sharedService = std::move(service);
        services[typeid(T)] = sharedService;
        GetModuleOrder().push_back(sharedService);
    }

    // Convenience overload for legacy call sites: Provide(new T(...)).
    // Wraps immediately so rejection paths cannot leak.
    template <typename T> static void Provide(T* service)
    {
        Provide(std::unique_ptr<T>(service));
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

        if (s_IsLocked)
        {
            CH_CORE_ERROR("ServiceLocator: ProvideRef<{}> rejected — locator is locked.", typeid(T).name());
            return;
        }

        auto& services = GetInternalMap();
        if (services.find(typeid(T)) != services.end())
        {
            CH_CORE_ERROR("ServiceLocator: ProvideRef<{}> rejected — service already provided.", typeid(T).name());
            return;
        }
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
        // Copy the module list under the lock, then iterate WITHOUT holding it.
        // A module's Initialize() may legitimately call Get()/TryGet() (and re-locking
        // a shared_mutex on the same thread is UB); holding the lock here would deadlock.
        std::vector<std::shared_ptr<EngineModule>> modules;
        {
            std::shared_lock<std::shared_mutex> lock(GetMutex());
            modules = GetModuleOrder();
        }

        for (auto& module : modules)
        {
            if (!module->IsEnabled())
            {
                continue;
            }

            try
            {
                module->Initialize();
            } catch (const std::exception& e)
            {
                CH_CORE_ERROR("ServiceLocator: Module initialization failed with exception: {}", e.what());
                module->SetEnabled(false);
            } catch (...)
            {
                CH_CORE_ERROR("ServiceLocator: Module initialization failed with an unknown exception.");
                module->SetEnabled(false);
            }
        }
    }

    static void Shutdown()
    {
        std::vector<std::shared_ptr<EngineModule>> modulesToShutdown;
        {
            std::unique_lock<std::shared_mutex> lock(GetMutex());
            s_IsLocked = false;
            s_IsShuttingDown = true;
            modulesToShutdown = GetModuleOrder();
        }

        for (auto it = modulesToShutdown.rbegin(); it != modulesToShutdown.rend(); ++it)
        {
            if ((*it)->IsEnabled())
            {
                (*it)->Shutdown();
            }
            // Mark as disabled so Get()/TryGet() report this module as unavailable for
            // the rest of the shutdown sequence (reverse order: earlier modules may still
            // legitimately look up their still-alive dependencies, but nobody must ever
            // receive an already-shut-down service).
            (*it)->SetEnabled(false);
        }

        {
            std::unique_lock<std::shared_mutex> lock(GetMutex());
            GetInternalMap().clear();
            GetModuleOrder().clear();
            s_IsShuttingDown = false;
        }
    }
    static bool IsAvailable()
    {
        std::shared_lock<std::shared_mutex> lock(GetMutex());
        return !GetInternalMap().empty();
    }
    // Contract (shared with TryGet): a disabled module (e.g. its Initialize() threw) is
    // treated as UNAVAILABLE — both methods return nullptr for it. Get() additionally
    // asserts, because callers of Get() declare a hard dependency.
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
                if (s_IsShuttingDown)
                {
                    // Use-after-shutdown: an earlier module is looking up a dependency
                    // that has already been shut down (reverse order violation).
                    CH_CORE_ERROR("ServiceLocator: Get<{}> during shutdown — service already shut down.",
                                  typeid(T).name());
                }
                else
                {
                    CH_CORE_ASSERT(false, "ServiceLocator: Requested service is disabled (initialization failed?)!");
                    CH_CORE_ERROR("ServiceLocator: Get<{}> returning nullptr — service is disabled.", typeid(T).name());
                }
                return nullptr;
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
            {
                return svc;
            }
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
    inline static bool s_IsShuttingDown = false;
};
} // namespace Chained

#endif // CH_SERVICE_LOCATOR_H