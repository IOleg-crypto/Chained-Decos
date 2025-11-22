//
// EngineApplication.h - Base application class for all projects using the engine
// Created by Auto on 2025
//
#ifndef ENGINE_APPLICATION_H
#define ENGINE_APPLICATION_H

#include "Engine/Engine.h"
#include "core/object/kernel/Core/Kernel.h"
#include "core/object/module/Core/ModuleManager.h"
#include "IApplication.h"
#include <functional>
#include <memory>
#include <string>

// Engine Runtime - Runs the application
// Manages the lifecycle of the engine and delegates application logic to IApplication
class EngineApplication
{
public:
    struct Config
    {
        int width;
        int height;
        std::string windowName;
        bool enableMSAA;
        bool resizable;

        Config()
            : width(1280), height(720), windowName("Engine Application"), enableMSAA(true),
              resizable(true)
        {
        }
    };

    // Constructor now takes the application instance
    EngineApplication(IApplication *app, const Config &config = Config());
    ~EngineApplication();

    // Main lifecycle loop (called from main)
    void Run();

    // Public API for engine access
    Engine *GetEngine() const
    {
        return m_engine.get();
    }
    Kernel *GetKernel() const
    {
        return m_kernel.get();
    }
    ModuleManager *GetModuleManager() const
    {
        return m_engine ? m_engine->GetModuleManager() : nullptr;
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
    std::unique_ptr<Kernel> m_kernel;
    std::unique_ptr<Engine> m_engine;
    bool m_initialized = false;
};

#endif // ENGINE_APPLICATION_H
