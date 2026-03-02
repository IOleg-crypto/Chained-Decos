#ifndef CH_ENTRY_POINT_H
#define CH_ENTRY_POINT_H

#include "engine/core/application.h"

extern CHEngine::Application* CHEngine::CreateApplication(CHEngine::ApplicationCommandLineArgs args);

// Main entry point for the engine and application.
int main(int argc, char** argv)
{
    CHEngine::ApplicationCommandLineArgs args;
    args.Count = argc;
    args.Args = argv;

    auto app = CHEngine::CreateApplication(args);
    app->Run();
    delete app;

    return 0;
}

#if defined(CH_PLATFORM_WINDOWS) && !defined(CH_DEBUG)
#include <windows.h>
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    return main(__argc, __argv);
}
#endif

#endif // CH_ENTRY_POINT_H
