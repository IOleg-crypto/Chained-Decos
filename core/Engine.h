#ifndef ENGINE_H
#define ENGINE_H

#include <memory>
#include <string>
#include <vector>

// Forward declarations of core systems
#include "components/audio/Core/AudioManager.h"
#include "components/audio/Interfaces/IAudioManager.h"
#include "components/input/Core/InputManager.h"
#include "components/input/Interfaces/IInputManager.h"
#include "components/physics/collision/Interfaces/ICollisionManager.h"
#include "components/rendering/Core/RenderManager.h"
#include "core/interfaces/IEngineModule.h"
#include "core/interfaces/ILevelManager.h"
#include "core/interfaces/IMenu.h"
#include "core/interfaces/IPlayer.h"
#include "core/module/ModuleManager.h"
#include "scene/main/Interfaces/IWorldManager.h"
#include "scene/resources/model/Interfaces/IModelLoader.h"

// Concrete managers for service locator implementation
#include "components/physics/collision/Core/CollisionManager.h"
#include "scene/main/Core/World.h"
#include "scene/resources/model/Core/Model.h"

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
    std::shared_ptr<RenderManager> GetRenderManager() const;
    std::shared_ptr<IInputManager> GetInputManager() const
    {
        return m_InputManager;
    }
    std::shared_ptr<AudioManager> GetAudioManager() const;
    std::shared_ptr<IModelLoader> GetModelLoader() const;
    std::shared_ptr<ICollisionManager> GetCollisionManager() const;
    std::shared_ptr<IWorldManager> GetWorldManager() const;
    std::shared_ptr<ILevelManager> GetLevelManager() const override;
    std::shared_ptr<IPlayer> GetPlayer() const override;
    std::shared_ptr<IMenu> GetMenu() const override;

    ModuleManager *GetModuleManager() const;

    // Service Locator (Shim for transition - to be removed)
    template <typename T> std::shared_ptr<T> GetService() const;
    template <typename T> void RegisterService(std::shared_ptr<T> service);

    // Module registration
    void RegisterModule(std::unique_ptr<IEngineModule> module);

    // Debug
    bool IsDebugInfoVisible() const;
    void SetDebugInfoVisible(bool visible);
    bool IsCollisionDebugVisible() const;

    // Application control
    void RequestExit() override;
    bool ShouldExit() const override;

private:
    static Engine *s_instance;

    std::unique_ptr<ModuleManager> m_ModuleManager;

    // Explicit Core Services
    std::shared_ptr<RenderManager> m_RenderManager;
    std::shared_ptr<ChainedDecos::InputManager> m_InputManager;
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
template <typename T> inline void Engine::RegisterService(std::shared_ptr<T> service)
{
    if constexpr (std::is_same_v<T, ILevelManager>)
        m_LevelManager = service;
    else if constexpr (std::is_same_v<T, IPlayer>)
        m_Player = service;
    else if constexpr (std::is_same_v<T, IMenu>)
        m_Menu = service;
    else if constexpr (std::is_same_v<T, IModelLoader> || std::is_same_v<T, ModelLoader>)
        m_ModelLoader = std::static_pointer_cast<IModelLoader>(service);
    else if constexpr (std::is_same_v<T, ICollisionManager> || std::is_same_v<T, CollisionManager>)
        m_CollisionManager = std::static_pointer_cast<ICollisionManager>(service);
    else if constexpr (std::is_same_v<T, IWorldManager> || std::is_same_v<T, WorldManager>)
        m_WorldManager = std::static_pointer_cast<IWorldManager>(service);
}

template <typename T> inline std::shared_ptr<T> Engine::GetService() const
{
    if constexpr (std::is_same_v<T, ILevelManager>)
        return std::static_pointer_cast<T>(m_LevelManager);
    else if constexpr (std::is_same_v<T, IPlayer>)
        return std::static_pointer_cast<T>(m_Player);
    else if constexpr (std::is_same_v<T, IMenu>)
        return std::static_pointer_cast<T>(m_Menu);
    else if constexpr (std::is_same_v<T, IModelLoader> || std::is_same_v<T, ModelLoader>)
        return std::static_pointer_cast<T>(m_ModelLoader);
    else if constexpr (std::is_same_v<T, ICollisionManager> || std::is_same_v<T, CollisionManager>)
        return std::static_pointer_cast<T>(m_CollisionManager);
    else if constexpr (std::is_same_v<T, IWorldManager> || std::is_same_v<T, WorldManager>)
        return std::static_pointer_cast<T>(m_WorldManager);
    else if constexpr (std::is_same_v<T, IInputManager> ||
                       std::is_same_v<T, ChainedDecos::InputManager>)
        return std::static_pointer_cast<T>(m_InputManager);
    else if constexpr (std::is_same_v<T, IAudioManager> || std::is_same_v<T, AudioManager>)
        return std::static_pointer_cast<T>(m_AudioManager);

    return nullptr;
}

#endif // ENGINE_H
