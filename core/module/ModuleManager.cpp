#include "core/module/ModuleManager.h"
#include "Engine.h"
#include "interfaces/IEngineModule.h"
#include <algorithm>
#include <functional>
#include <raylib.h>
#include <set>

ModuleManager::ModuleManager() : m_initialized(false)
{
}

ModuleManager::~ModuleManager()
{
    if (m_initialized)
    {
        ShutdownAllModules();
    }
}

void ModuleManager::RegisterModule(std::unique_ptr<IEngineModule> module)
{
    if (!module)
    {
        return;
    }

    std::string moduleName = module->GetModuleName();

    if (m_modules.find(moduleName) != m_modules.end())
    {
        return;
    }

    m_modules[moduleName] = std::move(module);
    m_registrationOrder.push_back(moduleName);
}

bool ModuleManager::LoadModule(const std::string &moduleName)
{
    return IsModuleLoaded(moduleName);
}

bool ModuleManager::InitializeAllModules(ChainedEngine::Engine *engine)
{
    if (m_initialized)
    {
        return true;
    }

    if (!engine)
    {
        TraceLog(LOG_ERROR, "[ModuleManager] Cannot initialize modules with null engine");
        return false;
    }

    auto sortedModules = SortModulesByDependencies();

    for (auto *module : sortedModules)
    {
        if (!module)
        {
            continue;
        }

        auto deps = module->GetDependencies();
        std::string moduleName = module->GetModuleName();

        if (!CheckDependencies(moduleName, deps))
        {
            continue;
        }

        // First initialize the module (creates components)
        if (!module->Initialize(engine))
        {
            TraceLog(LOG_WARNING, "[ModuleManager] Failed to initialize module: %s",
                     moduleName.c_str());
            continue;
        }

        // Register services after initialization
        module->RegisterServices(engine);

        module->SetInitialized(true);
    }

    m_initialized = true;
    return true;
}

void ModuleManager::ShutdownAllModules()
{
    if (!m_initialized)
    {
        return;
    }

    // Shutdown in reverse registration order
    for (auto it = m_registrationOrder.rbegin(); it != m_registrationOrder.rend(); ++it)
    {
        auto modIt = m_modules.find(*it);
        if (modIt != m_modules.end() && modIt->second && modIt->second->IsInitialized())
        {
            modIt->second->Shutdown();
        }
    }

    m_initialized = false;
}

void ModuleManager::UpdateAllModules(float deltaTime)
{
    if (!m_initialized)
    {
        return;
    }

    for (const auto &name : m_registrationOrder)
    {
        auto it = m_modules.find(name);
        if (it != m_modules.end() && it->second && it->second->IsInitialized())
        {
            it->second->Update(deltaTime);
        }
    }
}

void ModuleManager::RenderAllModules()
{
    if (!m_initialized)
    {
        return;
    }

    for (const auto &name : m_registrationOrder)
    {
        auto it = m_modules.find(name);
        if (it != m_modules.end() && it->second && it->second->IsInitialized())
        {
            it->second->Render();
        }
    }
}

IEngineModule *ModuleManager::GetModule(const std::string &name) const
{
    auto it = m_modules.find(name);
    if (it != m_modules.end())
    {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> ModuleManager::GetLoadedModules() const
{
    return m_registrationOrder;
}

bool ModuleManager::IsModuleLoaded(const std::string &name) const
{
    return m_modules.find(name) != m_modules.end();
}

std::vector<IEngineModule *> ModuleManager::SortModulesByDependencies() const
{
    std::vector<IEngineModule *> sorted;
    std::set<std::string> visited;
    std::set<std::string> visiting;

    std::function<void(IEngineModule *)> visit = [&](IEngineModule *module)
    {
        if (!module)
            return;

        std::string name = module->GetModuleName();

        if (visited.count(name))
        {
            return;
        }

        if (visiting.count(name))
        {
            TraceLog(LOG_WARNING,
                     "[ModuleManager] Circular dependency detected involving module: %s",
                     name.c_str());
            return;
        }

        visiting.insert(name);

        auto deps = module->GetDependencies();
        for (const auto &depName : deps)
        {
            auto it = m_modules.find(depName);
            if (it != m_modules.end())
            {
                visit(it->second.get());
            }
        }

        visiting.erase(name);
        visited.insert(name);
        sorted.push_back(module);
    };

    for (const auto &pair : m_modules)
    {
        if (pair.second)
        {
            visit(pair.second.get());
        }
    }

    return sorted;
}

bool ModuleManager::CheckDependencies(const std::string &moduleName,
                                      const std::vector<std::string> &deps) const
{
    for (const auto &depName : deps)
    {
        auto *depModule = GetModule(depName);
        if (!depModule || !depModule->IsInitialized())
        {
            return false;
        }
    }
    return true;
}
