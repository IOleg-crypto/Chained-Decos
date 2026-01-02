#ifndef CD_CORE_SCRIPTING_SCRIPT_MANAGER_H
#define CD_CORE_SCRIPTING_SCRIPT_MANAGER_H

#include <entt/entt.hpp>
#include <memory>
#include <string>

namespace CHEngine
{
class ScriptManager
{
public:
    ScriptManager();
    ~ScriptManager();

    static void Init();
    static void Shutdown();
    static bool IsInitialized();

    static void Update(float deltaTime);

    static ScriptManager &Get()
    {
        return *s_Instance;
    }

    static void InitializeScripts(entt::registry &registry);
    static void UpdateScripts(entt::registry &registry, float deltaTime);

    static void SetActiveRegistry(entt::registry *registry);
    static entt::registry *GetActiveRegistry()
    {
        return s_Instance ? s_Instance->m_activeRegistry : nullptr;
    }

private:
    bool InternalInitialize();
    void InternalShutdown();
    void InternalUpdate(float deltaTime);
    void InternalInitializeScripts(entt::registry &registry);
    void InternalUpdateScripts(entt::registry &registry, float deltaTime);

private:
    bool m_initialized = false;
    entt::registry *m_activeRegistry = nullptr;

    static std::unique_ptr<ScriptManager> s_Instance;
};
} // namespace CHEngine

#endif // CD_CORE_SCRIPTING_SCRIPT_MANAGER_H
