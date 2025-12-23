#ifndef ENTRY_POINT_H
#define ENTRY_POINT_H

#include "core/Log.h"
#include "core/application/EngineApplication.h"
#include "raylib.h"
#include <exception>


// Main entry point macro for applications
// Handles initialization, main loop, and cleanup with error handling
#define ENGINE_MAIN(AppClass, AppName)                                                             \
    int main(int argc, char **argv)                                                                \
    {                                                                                              \
        try                                                                                        \
        {                                                                                          \
            AppClass app(argc, argv);                                                              \
            CHEngine::EngineApplication::Config config;                                        \
            config.windowName = AppName;                                                           \
            CHEngine::EngineApplication engine(config, &app);                                  \
            engine.Run();                                                                          \
            return 0;                                                                              \
        }                                                                                          \
        catch (const std::exception &e)                                                            \
        {                                                                                          \
            CD_CORE_FATAL("Unhandled exception: %s", e.what());                                    \
            return -1;                                                                             \
        }                                                                                          \
        catch (...)                                                                                \
        {                                                                                          \
            CD_CORE_FATAL("Unknown exception occurred");                                           \
            return -1;                                                                             \
        }                                                                                          \
    }

// Macro to declare application entry point in derived class
// Usage: Add DECLARE_APPLICATION(YourAppClass) at the end of your .cpp file
#define DECLARE_APPLICATION(AppClass)                                                              \
    CHEngine::IApplication *CHEngine::CreateApplication(int argc, char *argv[])            \
    {                                                                                              \
        return new AppClass(argc, argv);                                                           \
    }

// Alternative: CreateApplication pattern for manual control
// Defined by the client application
extern CHEngine::IApplication *CHEngine::CreateApplication(int argc, char *argv[]);

// Manual main entry point (used when not using ENGINE_MAIN macro)
#ifndef ENGINE_MAIN_DEFINED
#define ENGINE_MAIN_DEFINED
inline int EngineMain(int argc, char *argv[])
{
    // Create the client application
    CHEngine::IApplication *app = CHEngine::CreateApplication(argc, argv);

    // Initial config from application
    CHEngine::IApplication::EngineConfig appConfig;
    app->OnConfigure(appConfig);

    // Create Engine Runtime Config
    CHEngine::EngineApplication::Config engineConfig;
    engineConfig.windowName = appConfig.windowName;
    engineConfig.width = appConfig.width;
    engineConfig.height = appConfig.height;
    engineConfig.fullscreen = appConfig.fullscreen;
    engineConfig.vsync = appConfig.vsync;

    // Create Engine Application Wrapper
    CHEngine::EngineApplication engineApp(engineConfig, app);

    // Run the engine
    engineApp.Run();

    // Cleanup
    delete app;

    return 0;
}

// Auto-invoke if CreateApplication is defined
#ifndef ENGINE_NO_AUTO_MAIN
int main(int argc, char *argv[])
{
    return EngineMain(argc, argv);
}
#endif // ENGINE_NO_AUTO_MAIN
#endif // ENGINE_MAIN_DEFINED

#endif // ENTRY_POINT_H
