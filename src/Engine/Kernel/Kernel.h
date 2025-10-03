#ifndef KERNEL_H
#define KERNEL_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "IKernelService.h"

class Kernel
{
public:
    enum class ServiceType : uint32_t
    {
        Render,
        Input,
        Models,
        Collision,
        World,
        Physics,
        Custom1,
        Custom2
    };

    static Kernel &GetInstance();

    bool Initialize(const std::string &configFile = "");
    void Shutdown();
    void Update(float deltaTime);
    void Render();

    void SetConfigValue(const std::string &key, const std::string &value);
    std::string GetConfigValue(const std::string &key, const std::string &defaultValue = "") const;

    template <typename T>
    void RegisterService(ServiceType type, std::shared_ptr<T> service)
    {
        static_assert(std::is_base_of_v<IKernelService, T>, "Service must implement IKernelService");
        m_services[type] = service;
    }

    template <typename T>
    std::shared_ptr<T> GetService(ServiceType type)
    {
        auto it = m_services.find(type);
        if (it == m_services.end())
            return nullptr;
        return std::dynamic_pointer_cast<T>(it->second);
    }

    void PrintServiceStatus();
    void Log(const std::string &message, int level = 3);

private:
    Kernel() = default;
    ~Kernel() = default;
    Kernel(const Kernel &) = delete;
    Kernel &operator=(const Kernel &) = delete;

    void InitializeServices();
    void ShutdownServices();

private:
    std::unordered_map<ServiceType, std::shared_ptr<void>> m_services;
    std::map<std::string, std::string> m_config;
};

#define REGISTER_KERNEL_SERVICE(Type, ServiceType) \
    Kernel::GetInstance().RegisterService<Type>(Kernel::ServiceType::ServiceType, std::make_shared<Type>())

#define GET_KERNEL_SERVICE(Type, ServiceType) \
    Kernel::GetInstance().GetService<Type>(Kernel::ServiceType::ServiceType)

#endif // KERNEL_H


