#include "ScriptSystem.h"
#include "core/scripting/ScriptManager.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/ScriptComponent.h"
#include <filesystem>

namespace CHEngine
{

void ScriptSystem::Update(entt::registry &registry, float deltaTime)
{
    if (!ScriptManager::IsInitialized())
        return;

    auto view = registry.view<ScriptComponent>();
    for (auto entity : view)
    {
        auto &script = view.get<ScriptComponent>(entity);
        if (!script.isEnabled || script.scriptPath.empty())
            continue;

        // Simple hot-reloading check
        if (std::filesystem::exists(script.scriptPath))
        {
            auto lastWrite =
                std::filesystem::last_write_time(script.scriptPath).time_since_epoch().count();
            if (script.lastModified != 0 && lastWrite > script.lastModified)
            {
                ScriptManager::RunScript(script.scriptPath);
            }
            script.lastModified = lastWrite;
        }

        // TODO: Call 'OnUpdate' from Lua if it exists for this entity
        // We need a way to link ECS entity to Lua object
    }
}

void ScriptSystem::OnStart(entt::registry &registry)
{
    if (!ScriptManager::IsInitialized())
        return;

    auto view = registry.view<ScriptComponent>();
    for (auto entity : view)
    {
        auto &script = view.get<ScriptComponent>(entity);
        if (script.isEnabled && !script.scriptPath.empty())
        {
            ScriptManager::RunScript(script.scriptPath);
            // TODO: Call 'OnStart' in Lua
        }
    }
}

} // namespace CHEngine
