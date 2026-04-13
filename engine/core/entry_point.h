#ifndef CH_ENTRY_POINT_H
#define CH_ENTRY_POINT_H

#include "engine/core/application.h"
#include "engine/scene/project.h"
#include "engine/core/base.h"
#include <memory>

extern CHEngine::Application* CHEngine::CreateApplication(CHEngine::ApplicationCommandLineArgs args);

// Main entry point for the engine and application.
int main(int argc, char** argv)
{
    CHEngine::ApplicationCommandLineArgs args;
    args.Count = argc;
    args.Args = argv;

    std::unique_ptr<CHEngine::Application> app(CHEngine::CreateApplication(args));
    app->Run();

    return 0;
}

#if CH_PLATFORM_WINDOWS && !defined(CH_DEBUG)
#include <windows.h>
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
#if defined(_MSC_VER)
    return main(__argc, __argv);
#else
    return main(0, nullptr); // MinGW might need a different approach if arguments are needed
#endif
}
#endif

#endif // CH_ENTRY_POINT_H
