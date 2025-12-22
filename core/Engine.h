#ifndef ENGINE_H
#define ENGINE_H

#include <memory>
#include <string>
#include <vector>

#include "core/module/ModuleManager.h"

// Manager Interfaces
#include "components/audio/interfaces/IAudioManager.h"
#include "components/input/interfaces/IInputManager.h"
#include "components/physics/collision/interfaces/ICollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/interfaces/IGuiManager.h"
#include "scene/main/interfaces/IWorldManager.h"
#include "scene/resources/model/interfaces/IModelLoader.h"

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

    // Core System Accessors - now delegating to ServiceRegistry
    std::shared_ptr<RenderManager> GetRenderManager() const;
    std::shared_ptr<IInputManager> GetInputManager() const;
    std::shared_ptr<IAudioManager> GetAudioManager() const;
    std::shared_ptr<IModelLoader> GetModelLoader() const;
    std::shared_ptr<ICollisionManager> GetCollisionManager() const;
    std::shared_ptr<IWorldManager> GetWorldManager() const;
    std::shared_ptr<IGuiManager> GetGuiManager() const;

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
    Window *GetWindow() const
    {
        return m_Window.get();
    }

private:
    static Engine *s_instance;

    std::unique_ptr<ModuleManager> m_ModuleManager;
    std::unique_ptr<Window> m_Window;

    bool m_debugInfoVisible = false;
    bool m_shouldExit = false;
};
} // namespace ChainedEngine

#include "core/ServiceRegistry.h"

template <typename T> inline void ChainedEngine::Engine::RegisterService(std::shared_ptr<T> service)
{
    ServiceRegistry::Register<T>(service);
}

template <typename T> inline std::shared_ptr<T> ChainedEngine::Engine::GetService() const
{
    return ServiceRegistry::Get<T>();
}

#endif // ENGINE_H
