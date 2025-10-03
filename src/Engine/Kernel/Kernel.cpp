#include "Kernel.h"
#include <raylib.h>

Kernel &Kernel::GetInstance()
{
    static Kernel instance;
    return instance;
}

bool Kernel::Initialize(const std::string &configFile)
{
    (void)configFile;
    InitializeServices();
    return true;
}

void Kernel::Shutdown()
{
    ShutdownServices();
}

void Kernel::Update(float deltaTime)
{
    for (auto &kv : m_services)
    {
        if (auto kernelService = std::static_pointer_cast<IKernelService>(kv.second))
        {
            kernelService->Update(deltaTime);
        }
    }
}

void Kernel::Render()
{
    for (auto &kv : m_services)
    {
        if (auto kernelService = std::static_pointer_cast<IKernelService>(kv.second))
        {
            kernelService->Render();
        }
    }
}

void Kernel::SetConfigValue(const std::string &key, const std::string &value)
{
    m_config[key] = value;
}

std::string Kernel::GetConfigValue(const std::string &key, const std::string &defaultValue) const
{
    auto it = m_config.find(key);
    if (it == m_config.end())
        return defaultValue;
    return it->second;
}

void Kernel::PrintServiceStatus()
{
    for (auto &kv : m_services)
    {
        const char *serviceName = "Unknown";
        if (auto kernelService = std::static_pointer_cast<IKernelService>(kv.second))
        {
            serviceName = kernelService->GetName();
        }
        TraceLog(LOG_INFO, "[Kernel] Service active: %s", serviceName);
    }
}

void Kernel::Log(const std::string &message, int level)
{
    TraceLog(static_cast<TraceLogLevel>(level), "%s", message.c_str());
}

void Kernel::InitializeServices()
{
    for (auto &kv : m_services)
    {
        if (auto kernelService = std::static_pointer_cast<IKernelService>(kv.second))
        {
            kernelService->Initialize();
        }
    }
}

void Kernel::ShutdownServices()
{
    for (auto it = m_services.begin(); it != m_services.end(); ++it)
    {
        if (auto kernelService = std::static_pointer_cast<IKernelService>(it->second))
        {
            kernelService->Shutdown();
        }
    }
}


