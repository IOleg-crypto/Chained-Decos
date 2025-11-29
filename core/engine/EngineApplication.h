#ifndef ENGINE_APPLICATION_H
#define ENGINE_APPLICATION_H

#include "Engine.h"
#include "IApplication.h"
#include <memory>
#include <string>

// Engine Runtime - Runs the application
// Manages the lifecycle of the engine and delegates application logic to IApplication
class EngineApplication
{
public:
    struct Config
    {
        int width = 1280;
        int height = 720;
        std::string windowName = "Engine Application";
        bool enableMSAA = true;
        bool resizable = true;
    };

    // Constructor now takes the application instance
    EngineApplication(Config config, IApplication *application);
    ~EngineApplication();

    // Main run loop
    void Run();

    // Accessors
    // Public API for engine access
    Engine *GetEngine() const
    {
        return m_engine.get();
    }

    // Configuration
    Config &GetConfig()
    {
        return m_config;
    }
    const Config &GetConfig() const
    {
        return m_config;
    }

private:
    void Initialize();
    void Shutdown();
    void Update();
    void Render();

    IApplication *m_app; // The application instance
    Config m_config;
    std::shared_ptr<Engine> m_engine; // Engine is now shared/singleton managed
    bool m_initialized = false;
};

#endif // ENGINE_APPLICATION_H
