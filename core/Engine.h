#ifndef ENGINE_H
#define ENGINE_H

#include <entt/entt.hpp>
#include <memory>
#include <string>
#include <vector>


#include "core/module/ModuleManager.h"

// Forward detections/Forward declarations are avoided as per user request
// but for Engine members we need the full types or pointers.
// Since we are cleaning up forward declarations later, I will add the necessary includes now.
#include "components/audio/interfaces/IAudioManager.h"
#include "components/input/core/InputManager.h"
#include "components/physics/collision/interfaces/ICollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/imgui/core/GuiManager.h"
#include "core/scripting/ScriptManager.h"
#include "events/UIEventRegistry.h"
#include "scene/SceneManager.h"
#include "scene/main/interfaces/IWorldManager.h"
#include "scene/resources/font/FontService.h"
#include "scene/resources/model/interfaces/IModelLoader.h"

#include "core/ServiceRegistry.h"
#include "core/interfaces/IEngine.h"
#include "core/window/Window.h" // For WindowProps

namespace CHEngine
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
    RenderManager &GetRenderManager() const override;
    CHEngine::InputManager &GetInputManager() const override;
    IAudioManager &GetAudioManager() const override;
    IModelLoader &GetModelLoader() const override;
    ICollisionManager &GetCollisionManager() const override;
    IWorldManager &GetWorldManager() const override;
    ScriptManager &GetScriptManager() const;
    CHEngine::GuiManager &GetGuiManager() const override;
    CHEngine::SceneManager &GetSceneManager() const override;
    CHEngine::FontService &GetFontService() const override;
    CHEngine::UIEventRegistry &GetUIEventRegistry() const override;
    entt::registry &GetECSRegistry() override;

    class ModuleManager *GetModuleManager() const;

    // Module registration
    void RegisterModule(std::unique_ptr<IEngineModule> module);

    // Generic Service Locator
    template <typename T> std::shared_ptr<T> GetService() const;
    template <typename T> void RegisterService(std::shared_ptr<T> service);

    // Debug
    bool IsDebugInfoVisible() const;
    void SetDebugInfoVisible(bool visible);
    bool IsCollisionDebugVisible() const;

    // Application control
    void RequestExit() override;
    bool ShouldExit() const override;

    // Window Access
    Window *GetWindow() const;

private:
    static Engine *s_instance;

    std::unique_ptr<ModuleManager> m_ModuleManager;
    std::unique_ptr<Window> m_Window;

    // Managers
    std::unique_ptr<RenderManager> m_RenderManager;
    std::unique_ptr<CHEngine::InputManager> m_InputManager;
    std::unique_ptr<IAudioManager> m_AudioManager;
    std::unique_ptr<ICollisionManager> m_CollisionManager;
    std::unique_ptr<IWorldManager> m_WorldManager;
    std::unique_ptr<IModelLoader> m_ModelLoader;
    std::unique_ptr<ScriptManager> m_ScriptManager;
    std::unique_ptr<CHEngine::GuiManager> m_GuiManager;
    std::unique_ptr<CHEngine::SceneManager> m_SceneManager;
    std::unique_ptr<CHEngine::FontService> m_FontService;
    std::unique_ptr<CHEngine::UIEventRegistry> m_UIEventRegistry;
    entt::registry m_ECSRegistry;

    bool m_debugInfoVisible = false;
    bool m_shouldExit = false;
};

} // namespace CHEngine

template <typename T> inline void CHEngine::Engine::RegisterService(std::shared_ptr<T> service)
{
    ServiceRegistry::Register<T>(service);
}

template <typename T> inline std::shared_ptr<T> CHEngine::Engine::GetService() const
{
    return ServiceRegistry::Get<T>();
}

#endif // ENGINE_H
