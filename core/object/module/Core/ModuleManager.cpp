#include "ModuleManager.h"
#include "../Interfaces/IEngineModule.h"
#include "core/object/kernel/Core/Kernel.h"
#include <raylib.h>
#include <algorithm>
#include <set>
#include <functional>

ModuleManager::ModuleManager(Kernel* kernel)
    : m_kernel(kernel), m_initialized(false)
{
}

ModuleManager::~ModuleManager()
{
    if (m_initialized) {
        ShutdownAllModules();
    }
}

void ModuleManager::RegisterModule(std::unique_ptr<IEngineModule> module)
{
    if (!module) {
        return;
    }
    
    std::string moduleName = module->GetModuleName();
    
    if (m_moduleByName.find(moduleName) != m_moduleByName.end()) {
        return;
    }
    
    IEngineModule* rawPtr = module.get();
    m_modules.push_back(std::move(module));
    m_moduleByName[moduleName] = rawPtr;
}

bool ModuleManager::LoadModule(const std::string& moduleName)
{
    return IsModuleLoaded(moduleName);
}

bool ModuleManager::InitializeAllModules()
{
    if (m_initialized) {
        return true;
    }
    
    if (!m_kernel) {
        return false;
    }
    
    auto sortedModules = SortModulesByDependencies();
    
    for (auto* module : sortedModules) {
        if (!module) {
            continue;
        }
        
        auto deps = module->GetDependencies();
        std::string moduleName = module->GetModuleName();
        
        if (!CheckDependencies(moduleName, deps)) {
            continue;
        }
        
        // First initialize the module (creates components)
        // Module can call RegisterServices inside Initialize if needed
        if (!module->Initialize(m_kernel)) {
            TraceLog(LOG_WARNING, "[ModuleManager] Failed to initialize module: %s", moduleName.c_str());
            continue;
        }
        
        // Register services after initialization (components created)
        // This allows modules to register their services after component creation
        module->RegisterServices(m_kernel);
        
        module->SetInitialized(true);
    }
    
    m_initialized = true;
    return true;
}

void ModuleManager::ShutdownAllModules()
{
    if (!m_initialized) {
        return;
    }
    
    for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it) {
        if (*it && (*it)->IsInitialized()) {
            (*it)->Shutdown();
        }
    }
    
    m_initialized = false;
}

void ModuleManager::UpdateAllModules(float deltaTime)
{
    for (auto& module : m_modules) {
        if (module && module->IsInitialized()) {
            module->Update(deltaTime);
        }
    }
}

void ModuleManager::RenderAllModules()
{
    for (auto& module : m_modules) {
        if (module && module->IsInitialized()) {
            module->Render();
        }
    }
}

IEngineModule* ModuleManager::GetModule(const std::string& name) const
{
    auto it = m_moduleByName.find(name);
    if (it != m_moduleByName.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> ModuleManager::GetLoadedModules() const
{
    std::vector<std::string> names;
    for (const auto& pair : m_moduleByName) {
        names.push_back(pair.first);
    }
    return names;
}

bool ModuleManager::IsModuleLoaded(const std::string& name) const
{
    return m_moduleByName.find(name) != m_moduleByName.end();
}

std::vector<IEngineModule*> ModuleManager::SortModulesByDependencies() const
{
    std::vector<IEngineModule*> sorted;
    std::set<std::string> visited;
    std::set<std::string> visiting;
    
    std::function<void(IEngineModule*)> visit = [&](IEngineModule* module) {
        if (!module) return;
        
        std::string name = module->GetModuleName();
        
        if (visited.count(name)) {
            return;
        }
        
        if (visiting.count(name)) {
            return;
        }
        
        visiting.insert(name);
        
        auto deps = module->GetDependencies();
        for (const auto& depName : deps) {
            auto* depModule = GetModule(depName);
            if (depModule) {
                visit(depModule);
            }
        }
        
        visiting.erase(name);
        visited.insert(name);
        sorted.push_back(module);
    };
    
    for (const auto& module : m_modules) {
        if (module) {
            visit(module.get());
        }
    }
    
    return sorted;
}

bool ModuleManager::CheckDependencies(const std::string& moduleName, const std::vector<std::string>& deps) const
{
    for (const auto& depName : deps) {
        auto* depModule = GetModule(depName);
        if (!depModule || !depModule->IsInitialized()) {
            return false;
        }
    }
    return true;
}

