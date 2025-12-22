#ifndef ENGINE_H
#define ENGINE_H

#include <any>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

// Forward declarations of core systems
#include "components/audio/core/AudioManager.h"
#include "components/audio/interfaces/IAudioManager.h"
#include "components/input/core/InputManager.h"
#include "components/input/interfaces/IInputManager.h"
#include "components/physics/collision/interfaces/ICollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/interfaces/IEngineModule.h"
#include "core/interfaces/IGuiManager.h"
#include "core/module/ModuleManager.h"
#include "scene/main/interfaces/IWorldManager.h"
#include "scene/resources/model/interfaces/IModelLoader.h"

// Concrete managers for service locator implementation
#include "components/physics/collision/core/collisionManager.h"
#include "scene/main/core/World.h"
#include "scene/resources/model/core/Model.h"

#include "core/interfaces/IEngine.h"

#include "core/window/Window.h" // For WindowProps

namespace ChainedEngine
{
class Engine : public IEngine
{
public:
    static Engine &Instance();

    Engine();
    ~Engine();

    // Core System Initialization
    bool Initialize(const WindowProps &props);

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
    std::shared_ptr<IGuiManager> GetGuiManager() const;

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

    // Window Access
    Window *GetWindow() const
    {
        return m_Window.get();
    }

private:
    static Engine *s_instance;

    std::unique_ptr<ModuleManager> m_ModuleManager;
    std::unique_ptr<Window> m_Window;

    // Explicit Core Services
    std::shared_ptr<RenderManager> m_RenderManager;
    std::shared_ptr<ChainedDecos::InputManager> m_InputManager;
    std::shared_ptr<AudioManager> m_AudioManager;
    std::shared_ptr<IModelLoader> m_ModelLoader;
    std::shared_ptr<ICollisionManager> m_CollisionManager;
    std::shared_ptr<IWorldManager> m_WorldManager;
    std::shared_ptr<IGuiManager> m_GuiManager;

    // Generic Service Map
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_Services;

    bool m_debugInfoVisible = false;
    bool m_shouldExit = false;
};
} // namespace ChainedEngine
template <typename T> inline void ChainedEngine::Engine::RegisterService(std::shared_ptr<T> service)
{
    m_Services[std::type_index(typeid(T))] = service;
}

template <typename T> inline std::shared_ptr<T> ChainedEngine::Engine::GetService() const
{
    auto it = m_Services.find(std::type_index(typeid(T)));
    if (it != m_Services.end())
    {
        return std::static_pointer_cast<T>(it->second);
    }

    // Fallback for core services
    if constexpr (std::is_same_v<T, IWorldManager>)
        return std::static_pointer_cast<T>(m_WorldManager);
    else if constexpr (std::is_same_v<T, IGuiManager>)
        return std::static_pointer_cast<T>(m_GuiManager);
    else if constexpr (std::is_same_v<T, IModelLoader> || std::is_same_v<T, ModelLoader>)
        return std::static_pointer_cast<T>(m_ModelLoader);
    else if constexpr (std::is_same_v<T, ICollisionManager> || std::is_same_v<T, CollisionManager>)
        return std::static_pointer_cast<T>(m_CollisionManager);
    else if constexpr (std::is_same_v<T, IInputManager> ||
                       std::is_same_v<T, ChainedDecos::InputManager>)
        return std::static_pointer_cast<T>(m_InputManager);
    else if constexpr (std::is_same_v<T, IAudioManager> || std::is_same_v<T, AudioManager>)
        return std::static_pointer_cast<T>(m_AudioManager);
    else if constexpr (std::is_same_v<T, RenderManager>)
        return std::static_pointer_cast<T>(m_RenderManager);

    return nullptr;
}

#endif // ENGINE_H
