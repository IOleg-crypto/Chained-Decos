#include "core/log.h"
#include "csharp/dot_net_host.h"
#include "csharp/script_glue.h"
#include "scene/ecs/components/scripting_components.h"
#include "scene/ecs/components/transform_component.h"
#include "script_manager.h"

namespace CHEngine
{
std::unique_ptr<ScriptManager> ScriptManager::s_Instance = nullptr;

ScriptManager::ScriptManager() : m_initialized(false), m_activeRegistry(nullptr)
{
}

ScriptManager::~ScriptManager()
{
}

void ScriptManager::Init()
{
    s_Instance = std::make_unique<ScriptManager>();
    s_Instance->InternalInitialize();
}

void ScriptManager::Shutdown()
{
    if (s_Instance)
        s_Instance->InternalShutdown();
    s_Instance.reset();
}

bool ScriptManager::IsInitialized()
{
    return s_Instance != nullptr && s_Instance->m_initialized;
}

void ScriptManager::Update(float deltaTime)
{
    if (s_Instance)
        s_Instance->InternalUpdate(deltaTime);
}

void ScriptManager::InitializeScripts(entt::registry &registry)
{
    if (s_Instance)
        s_Instance->InternalInitializeScripts(registry);
}

void ScriptManager::UpdateScripts(entt::registry &registry, float deltaTime)
{
    if (s_Instance)
        s_Instance->InternalUpdateScripts(registry, deltaTime);
}

void ScriptManager::SetActiveRegistry(entt::registry *registry)
{
    if (s_Instance)
        s_Instance->m_activeRegistry = registry;
}

bool ScriptManager::InternalInitialize()
{
    CD_CORE_INFO("Initializing Scripting System (C# only)...");

    // Initialize .NET Host
    if (DotNetHost::Init())
    {
        ScriptGlue::RegisterFunctions();
        m_initialized = true;
        CD_CORE_INFO("Scripting System initialized successfully.");
        return true;
    }

    CD_CORE_ERROR("Failed to initialize .NET Host.");
    return false;
}

void ScriptManager::InternalShutdown()
{
    CD_CORE_INFO("Shutting down Scripting System...");
    DotNetHost::Shutdown();
    m_initialized = false;
}

void ScriptManager::InternalUpdate(float deltaTime)
{
    if (!m_initialized || !m_activeRegistry)
        return;

    InternalUpdateScripts(*m_activeRegistry, deltaTime);
}

void ScriptManager::InternalInitializeScripts(entt::registry &registry)
{
    if (!m_initialized)
        return;

    auto view = registry.view<CSharpScriptComponent>();
    auto &delegates = ScriptGlue::GetDelegates();

    if (!delegates.CreateInstance)
        return;

    for (auto entity : view)
    {
        auto &script = view.get<CSharpScriptComponent>(entity);
        if (!script.initialized && !script.className.empty())
        {
            // Convert string to wchar_t for C# interop
            std::wstring wClassName(script.className.begin(), script.className.end());

            script.handle = delegates.CreateInstance((uint32_t)entity, (void *)wClassName.c_str());

            if (script.handle)
            {
                if (delegates.OnCreate)
                    delegates.OnCreate(script.handle);

                script.initialized = true;
                CD_CORE_INFO("C# Script initialized: %s for entity %d", script.className.c_str(),
                             (uint32_t)entity);
            }
            else
            {
                CD_CORE_ERROR("Failed to instantiate C# class: %s", script.className.c_str());
            }
        }
    }
}

void ScriptManager::InternalUpdateScripts(entt::registry &registry, float deltaTime)
{
    if (!m_initialized)
        return;

    auto view = registry.view<CSharpScriptComponent>();
    auto &delegates = ScriptGlue::GetDelegates();

    if (!delegates.OnUpdate)
        return;

    for (auto entity : view)
    {
        auto &script = view.get<CSharpScriptComponent>(entity);
        if (script.initialized && script.handle)
        {
            delegates.OnUpdate(script.handle, deltaTime);
        }
    }
}

} // namespace CHEngine
