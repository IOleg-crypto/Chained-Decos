#ifndef ENGINE_APPLICATION_H
#define ENGINE_APPLICATION_H

#include "Engine.h"
#include "IApplication.h"
#include "components/audio/Core/AudioManager.h"
#include "components/input/Core/InputManager.h"
#include "components/rendering/Core/RenderManager.h"
#include "core/macros.h"
#include <memory>
#include <string>

class EngineApplication
{
    DISABLE_COPY_AND_MOVE(EngineApplication)

public:
    struct Config
    {
        int width = 1280;
        int height = 720;
        std::string windowName = "Application";
    };

    EngineApplication(Config config, IApplication *app);
    ~EngineApplication();

    void Run();

private:
    void Initialize();
    void InitializeManagers();
    void InitializeWindow();
    void InitializeEngine();
    void InitializeApplication();
    void Shutdown();

    void Update();
    void Render();

    Config m_config;
    IApplication *m_app;

    // Managers (owned) - created in correct order
    std::unique_ptr<RenderManager> m_renderManager;
    std::unique_ptr<InputManager> m_inputManager;
    std::unique_ptr<AudioManager> m_audioManager;

    // Engine (depends on managers)
    std::unique_ptr<Engine> m_engine;

    bool m_initialized = false;
};

#endif // ENGINE_APPLICATION_H
