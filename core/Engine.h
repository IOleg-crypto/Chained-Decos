#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H

#include "Engine.h"

#include <string>

namespace Core
{

struct WindowConfig
{
    std::string title = "Game";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    bool fullscreen = false;
    int target_fps = 60;
};

struct EngineConfig
{
    WindowConfig window;
    bool enable_audio = true;
    bool enable_debug = false;
};

class Engine
{
public:
    Engine();
    ~Engine();

    // Non-copyable
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    // Run the application
    int Run(Application &app);

    // Request exit
    void RequestExit();

    // Check if running
    bool IsRunning() const;

private:
    bool Initialize(const EngineConfig &config);
    void Shutdown();
    void MainLoop(Application &app);

    bool m_running;
    EngineConfig m_config;
};

} // namespace Core

#endif // CORE_ENGINE_H
