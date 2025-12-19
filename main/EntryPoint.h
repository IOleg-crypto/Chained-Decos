#ifndef ENTRY_POINT_H
#define ENTRY_POINT_H

#include "core/application/EngineApplication.h"

// Simple macro to create main() function for any application
// Usage: ENGINE_MAIN(YourAppClass, "Your App Name")
#define ENGINE_MAIN(AppClass, AppName)                                                             \
    int main(int argc, char *argv[])                                                               \
    {                                                                                              \
        AppClass app(argc, argv);                                                                  \
        ChainedDecos::EngineApplication::Config config;                                            \
        config.windowName = AppName;                                                               \
        ChainedDecos::EngineApplication engine(config, &app);                                      \
        engine.Run();                                                                              \
        return 0;                                                                                  \
    }

#endif // ENTRY_POINT_H




