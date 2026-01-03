#ifndef CD_CORE_APPLICATION_ENTRY_POINT_H
#define CD_CORE_APPLICATION_ENTRY_POINT_H

#include "core/application/application.h"
#include "core/log.h"

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

#endif // CD_CORE_APPLICATION_ENTRY_POINT_H

#endif // CD_CORE_APPLICATION_ENTRY_POINT_H
