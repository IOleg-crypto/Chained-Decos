#ifndef ENTRY_POINT_H
#define ENTRY_POINT_H

#include "Application.h"
#include "core/Base.h"
#include "core/Log.h"

#ifdef CD_PLATFORM_WINDOWS

extern CHEngine::Application *CHEngine::CreateApplication(int argc, char **argv);

int main(int argc, char **argv)
{
    // CHEngine::Log::Init(); // If we need explicit init later

    CD_CORE_INFO("Engine Entry Point: Starting Application...");

    auto app = CHEngine::CreateApplication(argc, argv);
    app->Run();
    delete app;

    CD_CORE_INFO("Engine Entry Point: Application Shutdown.");
    return 0;
}

#endif

#endif // ENTRY_POINT_H
