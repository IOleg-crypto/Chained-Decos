#ifndef CH_ENTRY_POINT_H
#define CH_ENTRY_POINT_H

#include "engine/app/application.h"

extern Chained::Application* Chained::CreateApplication(Chained::ApplicationCommandLineArgs args);

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

#endif // CH_ENTRY_POINT_H
