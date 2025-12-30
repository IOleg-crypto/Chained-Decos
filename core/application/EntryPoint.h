#ifndef CHE_ENTRY_POINT_H
#define CHE_ENTRY_POINT_H

#include "core/Log.h"
#include "core/application/EngineApplication.h"
#include "core/application/IApplication.h"
#include <exception>

// To be defined by the client application
extern CHEngine::IApplication *CHEngine::CreateApplication(int argc, char *argv[]);

#ifndef CHE_ENTRY_POINT_DEFINED
#define CHE_ENTRY_POINT_DEFINED

int main(int argc, char *argv[])
{
    try
    {
        // 1. Create the client application instance
        CHEngine::IApplication *app = CHEngine::CreateApplication(argc, argv);
        if (!app)
        {
            CD_CORE_FATAL("Failed to create application!");
            return -1;
        }

        // 2. Initial configuration from the application
        CHEngine::IApplication::EngineConfig appConfig;
        app->OnConfigure(appConfig);

        // 3. Create Engine Runtime Config based on App requirements
        CHEngine::EngineApplication::Config engineConfig;
        engineConfig.windowName = appConfig.windowName;
        engineConfig.width = appConfig.width;
        engineConfig.height = appConfig.height;
        engineConfig.fullscreen = appConfig.fullscreen;
        engineConfig.vsync = appConfig.vsync;
        engineConfig.enableMSAA = true; // Default to true

        // 4. Create Engine Application Runner
        CHEngine::EngineApplication engineApp(engineConfig, app);

        // 5. Run the engine (Main Loop)
        engineApp.Run();

        // 6. Cleanup
        delete app;

        return 0;
    }
    catch (const std::exception &e)
    {
        CD_CORE_FATAL("Unhandled exception: %s", e.what());
        return -1;
    }
    catch (...)
    {
        CD_CORE_FATAL("Unknown exception occurred");
        return -1;
    }
}

// Optional helper macro for quick application declaration
#define DECLARE_APPLICATION(AppClass)                                                              \
    CHEngine::IApplication *CHEngine::CreateApplication(int argc, char *argv[])                    \
    {                                                                                              \
        return new AppClass(argc, argv);                                                           \
    }

#endif // CHE_ENTRY_POINT_DEFINED

#endif // CHE_ENTRY_POINT_H
