#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "core/interfaces/IEngineModule.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
class IEngine;
class IEngineModule;

/**
 * @brief ModuleManager - Handles engine module lifecycle
 */
class ModuleManager
{
public:
    static void Init();
    static void Shutdown();
    static bool IsInitialized();

    static void RegisterModule(std::unique_ptr<IEngineModule> module);
    static bool LoadModule(const std::string &moduleName);
    static void UpdateAll(float deltaTime);
    static void RenderAll();
    static IEngineModule *GetModule(const std::string &name);
    static std::vector<std::string> GetLoadedModules();
    static bool IsModuleLoaded(const std::string &name);

    ~ModuleManager();

private:
    explicit ModuleManager();

    void InternalRegisterModule(std::unique_ptr<IEngineModule> module);
    bool InternalLoadModule(const std::string &moduleName);
    void InternalShutdownAllModules();
    void InternalUpdateAllModules(float deltaTime);
    void InternalRenderAllModules();
    IEngineModule *InternalGetModule(const std::string &name) const;
    std::vector<std::string> InternalGetLoadedModules() const;
    bool InternalIsModuleLoaded(const std::string &name) const;

private:
    std::unordered_map<std::string, std::unique_ptr<IEngineModule>> m_modules;
    std::vector<std::string> m_registrationOrder;
    bool m_initialized;

    std::vector<IEngineModule *> SortModulesByDependencies() const;
    bool CheckDependencies(const std::string &moduleName,
                           const std::vector<std::string> &deps) const;
};
} // namespace CHEngine

#endif // MODULE_MANAGER_H
