#ifndef KERNEL_H
#define KERNEL_H

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "../Interfaces/IKernelService.h"

// SOLID-compliant Kernel using type-based service location
// Open/Closed: Can register any service type without modifying Kernel
// Interface Segregation: No need to know about all service types
// Dependency Inversion: Depends on IKernelService abstraction
class Kernel
{
public:
    bool Initialize(const std::string &configFile = "");
    void Shutdown();
    void Update(float deltaTime);
    void Render();

    void SetConfigValue(const std::string &key, const std::string &value);
    std::string GetConfigValue(const std::string &key, const std::string &defaultValue = "") const;

    // Global kernel instance access
    static Kernel& Instance();

    // Type-safe service registration without enum
    // Uses C++ RTTI for type identification
    template <typename T>
    void RegisterService(std::shared_ptr<T> service)
    {
        static_assert(std::is_base_of_v<IKernelService, T>, "Service must implement IKernelService");
        const std::type_index typeId = std::type_index(typeid(T));
        m_services[typeId] = std::static_pointer_cast<IKernelService>(service);
    }

    // Type-safe service retrieval
    template <typename T>
    std::shared_ptr<T> GetService()
    {
        const std::type_index typeId = std::type_index(typeid(T));
        auto it = m_services.find(typeId);
        if (it == m_services.end())
        {
            Log("Service not found: " + std::string(typeid(T).name()), 2); // LOG_WARNING
            return nullptr;
        }
        return std::static_pointer_cast<T>(it->second);
    }

    // Required service retrieval - throws if service is missing
    template <typename T>
    std::shared_ptr<T> RequireService()
    {
        const std::type_index typeId = std::type_index(typeid(T));
        auto it = m_services.find(typeId);
        if (it == m_services.end())
        {
            std::string errorMsg = "Required service not found: " + std::string(typeid(T).name());
            Log(errorMsg, 1); // LOG_ERROR
            throw std::runtime_error(errorMsg);
        }
        return std::static_pointer_cast<T>(it->second);
    }

    // Check if service exists
    template <typename T>
    bool HasService() const
    {
        const std::type_index typeId = std::type_index(typeid(T));
        return m_services.find(typeId) != m_services.end();
    }

    void PrintServiceStatus();
    void Log(const std::string &message, int level = 3);

    Kernel() = default;
    ~Kernel() = default;
    Kernel(const Kernel &) = delete;
    Kernel &operator=(const Kernel &) = delete;

    void InitializeServices();
    void ShutdownServices();

private:
    // Type-indexed service storage (no enum needed!)
    std::unordered_map<std::type_index, std::shared_ptr<IKernelService>> m_services;
    std::map<std::string, std::string> m_config;
    
    // Global instance
    static Kernel* s_instance;
};

// Simplified macros using type-based registration
#define REGISTER_KERNEL_SERVICE(kernel, Type) \
    kernel.RegisterService<Type>(std::make_shared<Type>())

#define GET_KERNEL_SERVICE(kernel, Type) \
    kernel.GetService<Type>()

// Global kernel access helper macros
#define KERNEL Kernel::Instance()
#define GET_SERVICE(Type) KERNEL.GetService<Type>()
#define REQUIRE_SERVICE(Type) KERNEL.RequireService<Type>()

#endif // KERNEL_H


