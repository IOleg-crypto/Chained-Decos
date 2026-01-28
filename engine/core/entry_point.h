#ifndef CH_ENTRY_POINT_H
#define CH_ENTRY_POINT_H

#include "engine/core/application.h"
#include "engine/scene/script_registry.h"
#include "raylib.h"

extern CHEngine::Application *CHEngine::CreateApplication(int argc, char **argv);

int main(int argc, char **argv)
{
    SetExitKey(NULL); // To prevent raylib from exiting on escape
    auto app = CHEngine::CreateApplication(argc, argv);
    if (app->Initialize(app->GetConfig()))
    {
        app->Run();
    }
    delete app;
    return 0;
}

#endif // CH_ENTRY_POINT_H
