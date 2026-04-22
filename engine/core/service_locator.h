#ifndef CH_SERVICE_LOCATOR_H
#define CH_SERVICE_LOCATOR_H

#include <memory>
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
    static void Register(std::shared_ptr<T> service) {
        m_Services[typeid(T)] = std::static_pointer_cast<void>(service);
    }

    template<typename T>
    static T& Get() {
        auto it = m_Services.find(typeid(T));
        CH_CORE_ASSERT(it != m_Services.end(), "Service Locator: Service not registered!");
        return *std::static_pointer_cast<T>(it->second);
    }

    template<typename T>
    static bool Has() {
        return m_Services.find(typeid(T)) != m_Services.end();
    }

    static void Shutdown() {
        m_Services.clear();
    }

private:
    inline static std::unordered_map<std::type_index, std::shared_ptr<void>> m_Services;
};

} // namespace CHEngine

#endif // CH_SERVICE_LOCATOR_H
