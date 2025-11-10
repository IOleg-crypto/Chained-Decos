#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "../Interfaces/IEngineModule.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

class Kernel;

class ModuleManager {
public:
    explicit ModuleManager(Kernel* kernel);
    ~ModuleManager();
    
    void RegisterModule(std::unique_ptr<IEngineModule> module);
    bool LoadModule(const std::string& moduleName);
    bool InitializeAllModules();
    void ShutdownAllModules();
    void UpdateAllModules(float deltaTime);
    void RenderAllModules();
    IEngineModule* GetModule(const std::string& name) const;
    std::vector<std::string> GetLoadedModules() const;
    bool IsModuleLoaded(const std::string& name) const;
    
private:
    Kernel* m_kernel;
    std::vector<std::unique_ptr<IEngineModule>> m_modules;
    std::unordered_map<std::string, IEngineModule*> m_moduleByName;
    bool m_initialized;
    
    std::vector<IEngineModule*> SortModulesByDependencies() const;
    bool CheckDependencies(const std::string& moduleName, const std::vector<std::string>& deps) const;
};

#endif // MODULE_MANAGER_H

