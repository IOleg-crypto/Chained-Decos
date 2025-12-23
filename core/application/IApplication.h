#ifndef I_APPLICATION_H
#define I_APPLICATION_H

#include <string>

#include "core/Engine.h"
#include "core/module/ModuleManager.h"
#include "events/Event.h"

namespace CHEngine
{
class EngineApplication;
class IApplication;

// To be implemented by CLIENT
IApplication *CreateApplication(int argc, char *argv[]);

// Interface for all applications using the engine
// Implement this interface to define your application's behavior
class IApplication
{
    // ... (content is huge, I should use a block move or just wrap it)
    // Actually, I can just remove the early closing brace and add it at the end.
    // BUT I failed to match content last time.
    // Providing Full Content might be safer if the tool supports it, but file is small enough.
    // Total lines ~100.
    // Let's try matching the start and end of the block to wrap it.

public:
    IApplication() = default;
    IApplication(int argc, char *argv[])
    {
    } // Accept command line args
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

    // ImGui Render Loop (called between ImGui Begin/End)
    virtual void OnImGuiRender()
    {
    }

    // Shutdown Phase
    virtual void OnShutdown()
    {
    }

    // Event Handling
    virtual void OnEvent(CHEngine::Event &e)
    {
    }

    // Setters for core systems (called by EngineApplication)
    virtual void SetEngine(CHEngine::Engine *engine)
    {
        m_engine = engine;
    }

    virtual void SetAppRunner(CHEngine::EngineApplication *appRunner)
    {
        m_appRunner = appRunner;
    }

    CHEngine::Engine *GetEngine() const
    {
        return m_engine;
    }

    CHEngine::EngineApplication *GetAppRunner() const
    {
        return m_appRunner;
    }

protected:
    CHEngine::Engine *m_engine = nullptr;
    CHEngine::EngineApplication *m_appRunner = nullptr;
};

} // namespace CHEngine

#endif // I_APPLICATION_H
