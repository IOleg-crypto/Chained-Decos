#ifndef ENTRY_POINT_H
#define ENTRY_POINT_H

#include "core/engine/EngineApplication.h"
#include <memory>

// Simple macro to create main() function for any application
// Usage: ENGINE_MAIN(YourAppClass, "Your App Name")
#define ENGINE_MAIN(AppClass, AppName)                                                             \
    int main(int argc, char *argv[])                                                               \
    {                                                                                              \
        AppClass app(argc, argv);                                                                  \
        EngineApplication::Config config;                                                          \
        config.windowName = AppName;                                                               \
        EngineApplication engine(config, &app);                                                    \
        engine.Run();                                                                              \
        return 0;                                                                                  \
    }

#endif // ENTRY_POINT_H
