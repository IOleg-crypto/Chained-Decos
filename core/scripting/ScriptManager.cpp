#include "ScriptManager.h"
#include "components/physics/collision/core/CollisionManager.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "core/interfaces/ILevelManager.h"
#include "events/Event.h"
#include "events/KeyEvent.h"
#include "events/UIEventRegistry.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/ScriptingComponents.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/VelocityComponent.h"
#include "scene/ecs/components/playerComponent.h"
#include <raylib.h>
#include <raymath.h>

namespace CHEngine
{

ScriptManager::ScriptManager()
{
}

ScriptManager::~ScriptManager()
{
}

bool ScriptManager::Initialize()
{
    CD_CORE_INFO("Initializing Scripting System (Lua)...");

    try
    {
        m_lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string,
                             sol::lib::table, sol::lib::os);
        BindEngineAPI();
        m_initialized = true;
        CD_CORE_INFO("Scripting System initialized successfully.");
        return true;
    }
    catch (const std::exception &e)
    {
        CD_CORE_ERROR("Failed to initialize Scripting System: %s", e.what());
        return false;
    }
}

void ScriptManager::Shutdown()
{
    CD_CORE_INFO("Shutting down Scripting System...");
    m_initialized = false;
}

void ScriptManager::Update(float deltaTime)
{
    if (!m_initialized || !m_activeRegistry)
        return;

    // Update entity scripts
    UpdateScripts(*m_activeRegistry, deltaTime);
}

void ScriptManager::InitializeScripts(entt::registry &registry)
{
    if (!m_initialized)
        return;

    auto view = registry.view<LuaScriptComponent>();

    for (auto entity : view)
    {
        auto &script = view.get<LuaScriptComponent>(entity);
        if (!script.initialized && !script.scriptPath.empty())
        {
            // Load the script globally for now (simplest approach)
            if (RunScript(script.scriptPath))
            {
                CallLuaFunction(script.scriptPath, "OnInit", (uint32_t)entity);
                script.initialized = true;
            }
        }
    }
}

void ScriptManager::UpdateScripts(entt::registry &registry, float deltaTime)
{
    if (!m_initialized)
        return;

    auto view = registry.view<LuaScriptComponent>();

    for (auto entity : view)
    {
        auto &script = view.get<LuaScriptComponent>(entity);
        if (script.initialized && !script.scriptPath.empty())
        {
            CallLuaFunction(script.scriptPath, "OnUpdate", (uint32_t)entity, deltaTime);
        }
    }
}

void ScriptManager::CallLuaFunction(const std::string &scriptPath, const std::string &functionName,
                                    uint32_t entityId, float dt)
{
    sol::protected_function func = m_lua[functionName];
    if (func.valid())
    {
        auto result = (dt > 0.0f) ? func(entityId, dt) : func(entityId);
        if (!result.valid())
        {
            sol::error err = result;
            CD_CORE_ERROR("Lua Exception in %s:%s: %s", scriptPath.c_str(), functionName.c_str(),
                          err.what());
        }
    }
}

bool ScriptManager::RunScript(const std::string &path)
{
    if (!m_initialized)
        return false;

    auto result = m_lua.script_file(path, sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error err = result;
        CD_CORE_ERROR("Lua Script Error (%s): %s", path.c_str(), err.what());
        return false;
    }
    return true;
}

bool ScriptManager::RunString(const std::string &code)
{
    if (!m_initialized)
        return false;

    auto result = m_lua.script(code, sol::script_pass_on_error);
    if (!result.valid())
    {
        sol::error err = result;
        CD_CORE_ERROR("Lua String Error: %s", err.what());
        return false;
    }
    return true;
}

void ScriptManager::BindEngineAPI()
{
    // 1. Logging
    m_lua.set_function("LogInfo", [](const std::string &msg) { CD_INFO("[Lua] %s", msg.c_str()); });
    m_lua.set_function("LogWarn", [](const std::string &msg) { CD_WARN("[Lua] %s", msg.c_str()); });
    m_lua.set_function("LogError",
                       [](const std::string &msg) { CD_ERROR("[Lua] %s", msg.c_str()); });

    // 2. Bind Scene and UI APIs
    BindSceneAPI();
    BindUIAPI();
}

void ScriptManager::SetSceneManager(void *unused)
{
    // Deprecated for now, ScriptManager should use Engine services directly
}

void ScriptManager::RegisterButtonCallback(const std::string &buttonName, sol::function callback)
{
    m_buttonCallbacks[buttonName] = std::move(callback);
    CD_CORE_INFO("ScriptManager: Registered callback for button '%s'", buttonName.c_str());
}

void ScriptManager::BindSceneAPI()
{
    // Scene Management API
    m_lua.set_function(
        "LoadScene",
        [this](sol::object sceneRef)
        {
            auto levelManager = Engine::Instance().GetService<ILevelManager>();
            if (!levelManager)
            {
                CD_ERROR("[Lua] LoadScene failed: LevelManager service not found!");
                return;
            }

            if (sceneRef.is<std::string>())
            {
                std::string path = sceneRef.as<std::string>();
                CD_INFO("[Lua] Loading scene by name/path: %s", path.c_str());
                levelManager->LoadScene(path);
            }
            else if (sceneRef.is<int>())
            {
                int index = sceneRef.as<int>();
                CD_INFO("[Lua] Loading scene by index: %d", index);
                levelManager->LoadSceneByIndex(index);
            }
            else
            {
                CD_ERROR("[Lua] LoadScene failed: Invalid argument type. Expected string or int.");
                return;
            }

            // Sync ECS with new scene data
            if (levelManager)
            {
                levelManager->RefreshMapEntities();
                levelManager->RefreshUIEntities();
                CD_INFO("[Lua] Scene ECS entities refreshed.");
            }
        });

    m_lua.set_function("QuitGame",
                       []()
                       {
                           CD_INFO("[Lua] Quit game requested.");
                           Engine::Instance().RequestExit();
                       });

    // 3. Entity Manipulation API
    m_lua.set_function("GetTime", []() { return (float)GetTime(); });

    m_lua.set_function("GetPosition",
                       [this](uint32_t entityId) -> Vector3
                       {
                           if (!m_activeRegistry)
                               return {0, 0, 0};
                           auto entity = (entt::entity)entityId;
                           if (m_activeRegistry->all_of<TransformComponent>(entity))
                           {
                               return m_activeRegistry->get<TransformComponent>(entity).position;
                           }
                           return {0, 0, 0};
                       });

    m_lua.set_function("SetPosition",
                       [this](uint32_t entityId, float x, float y, float z)
                       {
                           if (!m_activeRegistry)
                               return;
                           auto entity = (entt::entity)entityId;
                           if (m_activeRegistry->all_of<TransformComponent>(entity))
                           {
                               m_activeRegistry->get<TransformComponent>(entity).position = {x, y,
                                                                                             z};
                           }
                       });
}

void ScriptManager::BindUIAPI()
{
    // UI Button Callback Registration
    m_lua.set_function("OnButtonClick",
                       [this](const std::string &buttonName, sol::function callback)
                       { RegisterButtonCallback(buttonName, std::move(callback)); });

    // UI Helper to trigger button callbacks (called from C++)
    m_lua.set_function("TriggerButtonCallback",
                       [this](const std::string &buttonName)
                       {
                           auto it = m_buttonCallbacks.find(buttonName);
                           if (it != m_buttonCallbacks.end())
                           {
                               try
                               {
                                   it->second(); // Call Lua function
                               }
                               catch (const std::exception &e)
                               {
                                   CD_ERROR("[Lua] Button callback error for '%s': %s",
                                            buttonName.c_str(), e.what());
                               }
                           }
                           else
                           {
                               CD_WARN("[Lua] No callback registered for button '%s'",
                                       buttonName.c_str());
                           }
                       });
}

void ScriptManager::BindGameplayAPI()
{
    // IsColliding(entityId): Simple AABB check against Player
    m_lua.set_function("IsColliding",
                       [this](uint32_t entityId) -> bool
                       {
                           if (!m_activeRegistry)
                               return false;
                           auto &registry = *m_activeRegistry;
                           auto entity = (entt::entity)entityId;
                           if (!registry.valid(entity))
                               return false;

                           // Get Entity Transform
                           Vector3 entPos = {0, 0, 0};
                           Vector3 entScale = {1, 1, 1};
                           if (registry.all_of<TransformComponent>(entity))
                           {
                               auto &t = registry.get<TransformComponent>(entity);
                               entPos = t.position;
                               entScale = t.scale;
                           }
                           else
                           {
                               return false;
                           }

                           // Construct AABB for Entity
                           // Assuming unit cube centered at position
                           Vector3 halfSize = {entScale.x * 0.5f, entScale.y * 0.5f,
                                               entScale.z * 0.5f};
                           BoundingBox entBox;
                           entBox.min = Vector3Subtract(entPos, halfSize);
                           entBox.max = Vector3Add(entPos, halfSize);

                           // Find Player
                           auto playerView = registry.view<PlayerComponent, TransformComponent>();
                           for (auto [playerEntity, player, playerTransform] : playerView.each())
                           {
                               // Construct Player AABB (approximate)
                               Vector3 playerPos = playerTransform.position;
                               Vector3 playerHalfSize = {0.5f, 1.0f, 0.5f}; // Approx player size
                               BoundingBox playerBox;
                               playerBox.min = Vector3Subtract(playerPos, playerHalfSize);
                               playerBox.max = Vector3Add(playerPos, playerHalfSize);

                               if (CheckCollisionBoxes(entBox, playerBox))
                               {
                                   return true;
                               }
                           }
                           return false; // No collision with player
                       });

    // RespawnPlayer(): Resets player to spawn position
    m_lua.set_function(
        "RespawnPlayer",
        [this]()
        {
            if (!m_activeRegistry)
                return;
            CD_INFO("[Lua] RespawnPlayer called.");
            auto view =
                m_activeRegistry->view<TransformComponent, VelocityComponent, PlayerComponent>();
            for (auto [entity, transform, velocity, player] : view.each())
            {
                transform.position = player.spawnPosition;
                velocity.velocity = {0, 0, 0};
                player.isGrounded = false;
                player.runTimer = 0;
                player.maxHeight = 0;
            }
        });
}

void ScriptManager::SetActiveRegistry(entt::registry *registry)
{
    m_activeRegistry = registry;
}
} // namespace CHEngine
