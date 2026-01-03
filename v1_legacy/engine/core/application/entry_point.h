#ifndef CD_ENGINE_CORE_APPLICATION_ENTRY_POINT_H
#define CD_ENGINE_CORE_APPLICATION_ENTRY_POINT_H

#include "core/log.h"
#include "engine/core/application/application.h"


#ifdef CD_PLATFORM_WINDOWS

extern CHEngine::Application *CHEngine::CreateApplication(int argc, char **argv);

int main(int argc, char **argv)
{
    CHEngine::Log::Init();
    CD_CORE_INFO("--- Chained Engine Initialized ---");

    auto app = CHEngine::CreateApplication(argc, argv);
    app->Run();
    delete app;

    CD_CORE_INFO("--- Chained Engine Shutdown ---");
    return 0;
}

#endif
#endif
