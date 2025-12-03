#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "../Interfaces/IEngineModule.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ModuleManager
{
public:
    explicit ModuleManager();
    ~ModuleManager();

    void RegisterModule(std::unique_ptr<IEngineModule> module);
    bool LoadModule(const std::string &moduleName);
    bool InitializeAllModules();
    void ShutdownAllModules();
    void UpdateAllModules(float deltaTime);
    void RenderAllModules();
    IEngineModule *GetModule(const std::string &name) const;
    std::vector<std::string> GetLoadedModules() const;
    bool IsModuleLoaded(const std::string &name) const;

private:
    std::unordered_map<std::string, std::unique_ptr<IEngineModule>> m_modules;
    std::vector<std::string> m_registrationOrder;
    bool m_initialized;

    std::vector<IEngineModule *> SortModulesByDependencies() const;
    bool CheckDependencies(const std::string &moduleName,
                           const std::vector<std::string> &deps) const;
};

#endif // MODULE_MANAGER_H
