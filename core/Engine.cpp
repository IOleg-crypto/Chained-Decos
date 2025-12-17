#include "Engine.h"
#include "Application.h"
#include <raylib.h>

namespace Core
{

Engine::Engine() : m_running(false)
{
}

Engine::~Engine()
{
    Shutdown();
}

int Engine::Run(Application &app)
{
    // Get configuration from application
    app.OnConfigure(m_config);

    // Initialize engine
    if (!Initialize(m_config))
    {
        return -1;
    }

    // Initialize application
    app.SetEngine(this);
    app.OnStart();

    // Run main loop
    m_running = true;
    MainLoop(app);

    // Shutdown
    app.OnShutdown();
    Shutdown();

    return 0;
}

void Engine::RequestExit()
{
    m_running = false;
}

bool Engine::IsRunning() const
{
    return m_running && !WindowShouldClose();
}

bool Engine::Initialize(const EngineConfig &config)
{
    // Initialize window
    if (config.window.vsync)
    {
        SetConfigFlags(FLAG_VSYNC_HINT);
    }

    InitWindow(config.window.width, config.window.height, config.window.title.c_str());

    if (!IsWindowReady())
    {
        return false;
    }

    SetTargetFPS(config.window.target_fps);

    // Initialize audio if enabled
    if (config.enable_audio)
    {
        InitAudioDevice();
    }

    return true;
}

void Engine::Shutdown()
{
    if (IsAudioDeviceReady())
    {
        CloseAudioDevice();
    }

    if (IsWindowReady())
    {
        CloseWindow();
    }
}

void Engine::MainLoop(Application &app)
{
    while (IsRunning())
    {
        float delta_time = GetFrameTime();

        // Update
        app.OnUpdate(delta_time);

        // Render using engine
        BeginDrawing();
        ClearBackground(RAYWHITE);
        app.OnRender();
        EndDrawing();
    }
}

} // namespace Core
