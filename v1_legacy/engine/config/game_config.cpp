#include "game_config.h"
#include "core/log.h"
#include <cstring>
#include <iostream>

GameConfig CommandLineHandler::ParseArguments(int argc, char *argv[])
{
    GameConfig config;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--width") == 0 && i + 1 < argc)
            config.width = atoi(argv[++i]);
        else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc)
            config.height = atoi(argv[++i]);
        else if (strcmp(argv[i], "--fullscreen") == 0)
            config.fullscreen = true;
        else if (strcmp(argv[i], "--dev") == 0)
            config.developer = true;
        else if (strcmp(argv[i], "--map") == 0 && i + 1 < argc)
            config.mapPath = argv[++i];
        else if (strcmp(argv[i], "--skip-menu") == 0)
            config.skipMenu = true;
    }
    return config;
}

void CommandLineHandler::ShowConfig(const GameConfig &config)
{
    CD_CORE_INFO("Game Configuration:");
    CD_CORE_INFO("  Resolution: %dx%d", config.width, config.height);
    CD_CORE_INFO("  Fullscreen: %s", config.fullscreen ? "Yes" : "No");
    CD_CORE_INFO("  Dev Mode:   %s", config.developer ? "Yes" : "No");
    CD_CORE_INFO("  Map Path:   %s", config.mapPath.empty() ? "Default" : config.mapPath.c_str());
}
