#ifndef ENGINE_H
#define ENGINE_H

#include "../object/module/Core/ModuleManager.h"
#include "core/interfaces/ILevelManager.h"
#include "core/interfaces/IMenu.h"
#include "core/interfaces/IPlayer.h"
#include <memory>

#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>

// Core system includes (replacing forward declarations)
#include "servers/input/Core/InputManager.h"
#include "servers/rendering/Core/RenderManager.h"

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

    // Static Singleton Accessors
    RenderManager *GetRenderManager() const;
    InputManager *GetInputManager() const;

    // Direct access to game objects (replaces service wrappers)
    IPlayer *GetPlayer() const;
    ILevelManager *GetLevelManager() const;
    IMenu *GetMenu() const;

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

    // Services map
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_services;

    bool m_debugInfoVisible = false;
    bool m_shouldExit = false;
};

#endif // ENGINE_H
