#ifndef ENGINE_H
#define ENGINE_H

#include <memory>
#include <string>
#include <vector>

// Forward declarations of core systems
class RenderManager;
class InputManager;
class AudioManager;
class IModelLoader;
class ICollisionManager;
class IWorldManager;
class ILevelManager;
class IPlayer;
class IMenu;
class ModuleManager;
class IEngineModule;

#include "core/interfaces/IEngine.h"

namespace ChainedDecos
{
class Event;
}

class Engine : public IEngine
{
public:
    static Engine &Instance();

    Engine();
    ~Engine();

    bool Initialize();
    void Update(float deltaTime);
    void Shutdown();

    // Core System Accessors
    std::shared_ptr<RenderManager> GetRenderManager() const
    {
        return m_RenderManager;
    }
    std::shared_ptr<InputManager> GetInputManager() const
    {
        return m_InputManager;
    }
    std::shared_ptr<AudioManager> GetAudioManager() const
    {
        return m_AudioManager;
    }
    std::shared_ptr<IModelLoader> GetModelLoader() const
    {
        return m_ModelLoader;
    }
    std::shared_ptr<ICollisionManager> GetCollisionManager() const
    {
        return m_CollisionManager;
    }
    std::shared_ptr<IWorldManager> GetWorldManager() const
    {
        return m_WorldManager;
    }
    std::shared_ptr<ILevelManager> GetLevelManager() const override
    {
        return m_LevelManager;
    }
    std::shared_ptr<IPlayer> GetPlayer() const override
    {
        return m_Player;
    }
    std::shared_ptr<IMenu> GetMenu() const override
    {
        return m_Menu;
    }

    ModuleManager *GetModuleManager() const
    {
        return m_ModuleManager.get();
    }

    // Service Locator (Shim for transition - to be removed)
    template <typename T> std::shared_ptr<T> GetService() const
    {
        return nullptr;
    }
    template <typename T> void RegisterService(std::shared_ptr<T> service)
    {
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
    void RequestExit() override
    {
        m_shouldExit = true;
    }
    bool ShouldExit() const override;

private:
    static Engine *s_instance;

    std::unique_ptr<ModuleManager> m_ModuleManager;

    // Explicit Core Services
    std::shared_ptr<RenderManager> m_RenderManager;
    std::shared_ptr<InputManager> m_InputManager;
    std::shared_ptr<AudioManager> m_AudioManager;
    std::shared_ptr<IModelLoader> m_ModelLoader;
    std::shared_ptr<ICollisionManager> m_CollisionManager;
    std::shared_ptr<IWorldManager> m_WorldManager;
    std::shared_ptr<ILevelManager> m_LevelManager;
    std::shared_ptr<IPlayer> m_Player;
    std::shared_ptr<IMenu> m_Menu;

    bool m_debugInfoVisible = false;
    bool m_shouldExit = false;
};

#endif // ENGINE_H
