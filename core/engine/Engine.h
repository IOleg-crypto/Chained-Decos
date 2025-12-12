#ifndef ENGINE_H
#define ENGINE_H

#include "components/audio/Core/AudioManager.h"
#include "components/input/Core/InputManager.h"
#include "components/rendering/Core/RenderManager.h"
#include "core/macros.h"
#include "core/object/Object.h"
#include "core/object/module/Core/ModuleManager.h"
#include <memory>


// Main Engine class with Pure Dependency Injection
// No singletons, no Service Locator - all dependencies explicit
class Engine : public Object
{
    REGISTER_CLASS(Engine, Object)
    DISABLE_COPY_AND_MOVE(Engine)

public:
    // Constructor and Destructor
    Engine(RenderManager &renderManager, InputManager &inputManager, AudioManager &audioManager);
    ~Engine();

    // Lifecycle
    bool Initialize();
    void Update(float deltaTime);
    void Shutdown();

    // Manager access
    RenderManager &GetRenderManager();
    InputManager &GetInputManager();
    AudioManager &GetAudioManager();
    ModuleManager &GetModuleManager();

    // Module management
    void RegisterModule(std::unique_ptr<IEngineModule> module);

    // Application control
    void RequestExit();
    bool ShouldExit() const;

    // Debug
    bool IsDebugInfoVisible() const;
    void SetDebugInfoVisible(bool visible);

private:
    // Dependencies (injected)
    RenderManager &m_renderManager;
    InputManager &m_inputManager;
    AudioManager &m_audioManager;

    // Owned objects
    std::unique_ptr<ModuleManager> m_moduleManager;

    // State
    bool m_debugInfoVisible;
    bool m_shouldExit;
};

#endif // ENGINE_H
