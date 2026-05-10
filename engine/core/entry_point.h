#ifndef CH_ENTRY_POINT_H
#define CH_ENTRY_POINT_H

#include "engine/core/application.h"

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args);

inline int RunEntryPoint(int argc, char** argv)
{
    ApplicationCommandLineArgs args;
    args.Count = argc;
    args.Args = argv;

    auto app = CreateApplication(args);
    app->Run();

    delete app;

    return 0;
}
} // namespace CHEngine

#endif // CH_ENTRY_POINT_H
