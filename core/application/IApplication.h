#ifndef I_APPLICATION_H
#define I_APPLICATION_H

#include <string>

#include "core/Engine.h"
#include "core/module/ModuleManager.h"

namespace ChainedDecos
{
class EngineApplication;
}

// Interface for all applications using the engine
// Implement this interface to define your application's behavior
class IApplication
{
public:
    virtual ~IApplication() = default;

    struct EngineConfig
    {
        std::string windowName = "Chained Decos Engine";
        std::string title = "Chained Decos Engine";
        int width = 1280;
        int height = 720;
        bool fullscreen = false;
        bool vsync = true;
        bool enableAudio = true;
    };

    // 1. Configuration Phase
    // Called before initialization. Use this to set window properties.
    virtual void OnConfigure(EngineConfig &config)
    {
    }

    // 2. Registration Phase
    // Called after Engine creation but before module initialization.
    // Use this to register your modules and services.
    virtual void OnRegister()
    {
    }

    // 3. Start Phase
    // Called after full initialization. The engine is ready.
    virtual void OnStart()
    {
    }

    // Update Loop
    virtual void OnUpdate(float deltaTime)
    {
    }

    // Render Loop
    virtual void OnRender()
    {
    }

    // Shutdown Phase
    virtual void OnShutdown()
    {
    }

    // Event Handling
    virtual void OnEvent(ChainedDecos::Event &e)
    {
    }

    // Setters for core systems (called by EngineApplication)
    virtual void SetEngine(Engine *engine)
    {
        m_engine = engine;
    }

    virtual void SetAppRunner(ChainedDecos::EngineApplication *appRunner)
    {
        m_appRunner = appRunner;
    }

    Engine *GetEngine() const
    {
        return m_engine;
    }

    ChainedDecos::EngineApplication *GetAppRunner() const
    {
        return m_appRunner;
    }

protected:
    Engine *m_engine = nullptr;
    ChainedDecos::EngineApplication *m_appRunner = nullptr;
};

#endif // I_APPLICATION_H


