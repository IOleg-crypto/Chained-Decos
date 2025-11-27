//
// EngineApplication.h - Base application class for all projects using the engine
// Created by Auto on 2025
//
#pragma once

#include "Engine.h"
#include "IApplication.h"
#include "core/object/kernel/Core/Kernel.h"
#include "core/object/module/Core/ModuleManager.h"
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
    Kernel *GetKernel() const
    {
        return m_kernel.get();
    }
    ModuleManager *GetModuleManager() const;

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
    std::shared_ptr<Engine> m_engine;
    bool m_initialized = false;
};
