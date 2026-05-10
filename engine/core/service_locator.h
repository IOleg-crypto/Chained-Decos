#ifndef CH_SERVICE_LOCATOR_H
#define CH_SERVICE_LOCATOR_H

#include <unordered_map>
#include <typeindex>
#include "engine/core/ch_assert.h"

namespace CHEngine {

/**
 * @brief ServiceLocator provides a central registry for engine subsystems.
 * This pattern helps decouple systems and facilitates unit testing by allowing 
 * implementation swaps (Dependency Inversion).
 */
class ServiceLocator {
public:
    template<typename T>
    static void Register(T* service) {
        CH_CORE_ASSERT(service != nullptr, "ServiceLocator: Cannot register a null service!");
        m_Services[typeid(T)] = static_cast<void*>(service);
    }

    template<typename T>
    static T& Get() {
        auto it = m_Services.find(typeid(T));
        if (it == m_Services.end() || it->second == nullptr) {
            CH_CORE_ERROR("Service Locator: Service '{}' not registered or already removed!", typeid(T).name());
        }
        CH_CORE_ASSERT(it != m_Services.end() && it->second != nullptr, "Service Locator: Service not available!");
        return *static_cast<T*>(it->second);
    }

    template<typename T>
    static bool Has() {
        auto it = m_Services.find(typeid(T));
        return it != m_Services.end() && it->second != nullptr;
    }

    template<typename T>
    static void Remove() {
        auto it = m_Services.find(typeid(T));
        if (it != m_Services.end()) {
            it->second = nullptr; // Null out before erasing to catch dangling access
            m_Services.erase(it);
        }
    }

    static void Shutdown() {
        m_Services.clear();
    }

private:
    static std::unordered_map<std::type_index, void*> m_Services;
};

} // namespace CHEngine

#endif // CH_SERVICE_LOCATOR_H
