#ifndef CH_ENTRY_POINT_H
#define CH_ENTRY_POINT_H

#include "engine/core/application.h"
#include <raylib.h>

extern CHEngine::Application *CHEngine::CreateApplication();

int main(int argc, char **argv)
{
    SetExitKey(NULL); // To prevent raylib from exiting on escape
    auto app = CHEngine::CreateApplication();
    app->Run();
    delete app;
    return 0;
}

#endif // CH_ENTRY_POINT_H
