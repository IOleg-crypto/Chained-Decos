#ifndef CH_ENTRY_POINT_H
#define CH_ENTRY_POINT_H

#include "engine/core/application.h"
#include "engine/foundation/platform_detection.h"

extern Chained::Application* Chained::CreateApplication(Chained::ApplicationCommandLineArgs args);

#if CH_PLATFORM_WINDOWS || CH_PLATFORM_LINUX
int main(int argc, char** argv)
{
    Chained::ApplicationCommandLineArgs args;
    args.Count = argc;
    args.Args = argv;

    auto app = Chained::CreateApplication(args);
    app->Run();
    delete app;
    return 0;
}
#else
    #error "Unsupported platform for entry point!"
#endif

#endif // CH_ENTRY_POINT_H
