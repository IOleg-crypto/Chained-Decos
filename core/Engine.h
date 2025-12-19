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
}

template <typename T> inline std::shared_ptr<T> Engine::GetService() const
{
    return nullptr;
}

#endif // ENGINE_H
