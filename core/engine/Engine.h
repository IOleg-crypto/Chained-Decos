#ifndef ENGINE_H
#define ENGINE_H

#include "core/object/module/Core/ModuleManager.h"
#include "core/object/module/Interfaces/IModule.h"
#include "servers/input/Interfaces/IInputManager.h"
#include "servers/rendering/Interfaces/IRenderManager.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>

// Forward declarations for game objects
class Player;
class PlayerController;
class LevelManager;
class Menu;

// Main Engine class acting as Service Locator and System Manager
class Engine
{
public:
    static Engine &Instance();

    Engine();
    ~Engine();

    bool Initialize();
    void Update(float deltaTime);
    void Shutdown();

    // Accessors for explicit core systems
    ModuleManager *GetModuleManager() const
    {
        return m_moduleManager.get();
    }
    IRenderManager *GetRenderManager() const
    {
        return m_renderManager.get();
    }
    IInputManager *GetInputManager() const
    {
        return m_inputManager.get();
    }

    // Direct access to game objects (replaces service wrappers)
    Player *GetPlayer() const;
    PlayerController *GetPlayerController() const;
    LevelManager *GetLevelManager() const;
    Menu *GetMenu() const;

    // Service Locator Pattern (Type-safe)
    template <typename T> void RegisterService(std::shared_ptr<T> service)
    {
        m_services[std::type_index(typeid(T))] = service;
    }

    template <typename T> std::shared_ptr<T> GetService() const
    {
        auto it = m_services.find(std::type_index(typeid(T)));
        if (it == m_services.end())
            return nullptr;
        return std::static_pointer_cast<T>(it->second);
    }

    template <typename T> std::shared_ptr<T> RequireService()
    {
        auto service = GetService<T>();
        if (!service)
            throw std::runtime_error("Required service not found: " +
                                     std::string(typeid(T).name()));
        return service;
    }

    // Module registration
    void RegisterModule(std::unique_ptr<IEngineModule> module);

    // Debug
    bool IsDebugInfoVisible() const
    {
        return m_debugInfoVisible;
    }
    void SetDebugInfoVisible(bool visible)
    {
        m_debugInfoVisible = visible;
    }
    bool IsCollisionDebugVisible() const;

    // Application control
    void RequestExit()
    {
        m_shouldExit = true;
    }
    bool ShouldExit() const
    {
        return m_shouldExit;
    }

private:
    static Engine *s_instance;

    std::unique_ptr<ModuleManager> m_moduleManager;
    std::unique_ptr<IRenderManager> m_renderManager;
    std::unique_ptr<IInputManager> m_inputManager;

    // Generic service storage
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_services;

    bool m_debugInfoVisible = false;
    bool m_shouldExit = false;
};

// Global access macros
#define ENGINE Engine::Instance()
#define GET_SERVICE(Type) ENGINE.GetService<Type>()
#define REQUIRE_SERVICE(Type) ENGINE.RequireService<Type>()

#endif // ENGINE_H
