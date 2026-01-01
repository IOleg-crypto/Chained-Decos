#include "core/module/ModuleManager.h"
#include "core/Log.h"
#include "core/interfaces/IEngineModule.h"
#include <algorithm>
#include <functional>
#include <set>

namespace CHEngine
{
static std::unique_ptr<ModuleManager> s_Instance = nullptr;

void ModuleManager::Init()
{
    s_Instance = std::unique_ptr<ModuleManager>(new ModuleManager());
    s_Instance->m_initialized = true;
}

bool ModuleManager::IsInitialized()
{
    return s_Instance != nullptr;
}

void ModuleManager::Shutdown()
{
    if (s_Instance)
    {
        s_Instance->InternalShutdownAllModules();
        s_Instance.reset();
    }
}

void ModuleManager::RegisterModule(std::unique_ptr<IEngineModule> module)
{
    s_Instance->InternalRegisterModule(std::move(module));
}

bool ModuleManager::LoadModule(const std::string &moduleName)
{
    return s_Instance->InternalLoadModule(moduleName);
}

void ModuleManager::UpdateAll(float deltaTime)
{
    s_Instance->InternalUpdateAllModules(deltaTime);
}

void ModuleManager::RenderAll()
{
    s_Instance->InternalRenderAllModules();
}

IEngineModule *ModuleManager::GetModule(const std::string &name)
{
    return s_Instance->InternalGetModule(name);
}

std::vector<std::string> ModuleManager::GetLoadedModules()
{
    return s_Instance->InternalGetLoadedModules();
}

bool ModuleManager::IsModuleLoaded(const std::string &name)
{
    return s_Instance->InternalIsModuleLoaded(name);
}

ModuleManager::ModuleManager() : m_initialized(false)
{
}

ModuleManager::~ModuleManager()
{
    InternalShutdownAllModules();
}

void ModuleManager::InternalRegisterModule(std::unique_ptr<IEngineModule> module)
{
    if (!module)
        return;

    std::string moduleName = module->GetModuleName();
    if (m_modules.find(moduleName) != m_modules.end())
        return;

    m_modules[moduleName] = std::move(module);
    m_registrationOrder.push_back(moduleName);

    // Auto-initialize for now as IEngine is being deprecated
    auto &m = m_modules[moduleName];
    m->Initialize();
    m->SetInitialized(true);
}

bool ModuleManager::InternalLoadModule(const std::string &moduleName)
{
    return InternalIsModuleLoaded(moduleName);
}

void ModuleManager::InternalShutdownAllModules()
{
    for (auto it = m_registrationOrder.rbegin(); it != m_registrationOrder.rend(); ++it)
    {
        auto modIt = m_modules.find(*it);
        if (modIt != m_modules.end() && modIt->second && modIt->second->IsInitialized())
        {
            modIt->second->Shutdown();
        }
    }
    m_modules.clear();
    m_registrationOrder.clear();
}

void ModuleManager::InternalUpdateAllModules(float deltaTime)
{
    for (const auto &name : m_registrationOrder)
    {
        auto it = m_modules.find(name);
        if (it != m_modules.end() && it->second && it->second->IsInitialized())
        {
            it->second->Update(deltaTime);
        }
    }
}

void ModuleManager::InternalRenderAllModules()
{
    for (const auto &name : m_registrationOrder)
    {
        auto it = m_modules.find(name);
        if (it != m_modules.end() && it->second && it->second->IsInitialized())
        {
            it->second->Render();
        }
    }
}

IEngineModule *ModuleManager::InternalGetModule(const std::string &name) const
{
    auto it = m_modules.find(name);
    return (it != m_modules.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> ModuleManager::InternalGetLoadedModules() const
{
    return m_registrationOrder;
}

bool ModuleManager::InternalIsModuleLoaded(const std::string &name) const
{
    return m_modules.find(name) != m_modules.end();
}
} // namespace CHEngine
