#ifndef CH_ENTRY_POINT_H
#define CH_ENTRY_POINT_H

#include "engine/core/application.h"
#include "engine/scene/script_registry.h"
#include "raylib.h"

extern CHEngine::Application *CHEngine::CreateApplication(CHEngine::ApplicationCommandLineArgs args);

// Main entry point for the engine and application.
int main(int argc, char **argv)
{
    CHEngine::ApplicationCommandLineArgs args;
    args.Count = argc;
    args.Args = argv;

    auto app = CHEngine::CreateApplication(args);
    app->Run();
    delete app;
    
    return 0;
}

#endif // CH_ENTRY_POINT_H
