#ifndef ENGINE_H
#define ENGINE_H

#include <entt/entt.hpp>
#include <memory>

#include "core/ServiceRegistry.h"
#include "core/interfaces/IEngine.h"
#include "core/window/Window.h" // For WindowProps

// Headers needed for covariant return types in virtual methods
#include "components/audio/interfaces/IAudioManager.h"
#include "components/input/interfaces/IInputManager.h"
#include "components/physics/collision/interfaces/ICollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/interfaces/IGuiManager.h"
#include "core/scripting/ScriptManager.h"
#include "events/UIEventRegistry.h"
#include "scene/SceneManager.h"
#include "scene/core/SceneManager.h" // Contains ECSSceneManager
#include "scene/main/IWorldManager.h"
#include "scene/resources/font/FontService.h"
#include "scene/resources/model/interfaces/IModelLoader.h"
#include "scene/resources/texture/TextureService.h"

class ModuleManager;
class IEngineModule;

namespace CHEngine
{
class EngineApplication;

/**
 * @brief Core Engine singleton that manages all major systems
 */
class Engine : public IEngine
{
public:
    static Engine &Instance();

    Engine();
    ~Engine();

    // --- Engine Lifecycle ---
public:
    bool Initialize(const WindowProps &props);
    void Update(float deltaTime);
    void Shutdown() const;

    // --- System Accessors (IEngine Overrides) ---
public:
    RenderManager &GetRenderManager() const override;
    IInputManager &GetInputManager() const override;
    IAudioManager &GetAudioManager() const override;
    CHEngine::IModelLoader &GetModelLoader() const override;
    ICollisionManager &GetCollisionManager() const override;
    IWorldManager &GetWorldManager() const override;
    IGuiManager &GetGuiManager() const override;
    CHEngine::SceneManager &GetSceneManager() const override;
    CHEngine::FontService &GetFontService() const override;
    CHEngine::TextureService &GetTextureService() const override;
    CHEngine::UIEventRegistry &GetUIEventRegistry() const override;
    entt::registry &GetECSRegistry() override;

    // --- Extended Accessors ---
public:
    ScriptManager &GetScriptManager() const;
    ECSSceneManager &GetECSSceneManager() const;
    class ModuleManager *GetModuleManager() const;
    Window *GetWindow() const;

    // --- Module & Service Management ---
public:
    void RegisterModule(std::unique_ptr<IEngineModule> module) const;

    template <typename T> std::shared_ptr<T> GetService() const;
    template <typename T> void RegisterService(std::shared_ptr<T> service);

    // --- Application Control ---
public:
    void RequestExit() override;
    bool ShouldExit() const override;

    EngineApplication *GetAppRunner() const override;
    void SetAppRunner(EngineApplication *appRunner);

    // --- Debug Utilities ---
public:
    bool IsDebugInfoVisible() const;
    void SetDebugInfoVisible(bool visible);
    bool IsCollisionDebugVisible() const;

    // --- Member Variables ---
private:
    static Engine *s_instance;

    std::unique_ptr<ModuleManager> m_ModuleManager;
    std::unique_ptr<Window> m_Window;

    entt::registry m_ECSRegistry;

    mutable bool m_initialized = false;
    bool m_debugInfoVisible = false;
    bool m_shouldExit = false;

    EngineApplication *m_AppRunner = nullptr;
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
