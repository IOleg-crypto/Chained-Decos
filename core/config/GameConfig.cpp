#include "GameConfig.h"
#include <cstring>
#include <raylib.h>

GameConfig CommandLineHandler::ParseArguments(int argc, char *argv[])
{
    GameConfig config;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--width") == 0 && i + 1 < argc)
        {
            config.width = std::atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc)
        {
            config.height = std::atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--fullscreen") == 0)
        {
            config.fullscreen = true;
        }
        else if (strcmp(argv[i], "--developer") == 0 || strcmp(argv[i], "-dev") == 0)
        {
            config.developer = true;
        }
        else if (strcmp(argv[i], "--map") == 0 && i + 1 < argc)
        {
            config.mapPath = argv[++i];
        }
        else if (strcmp(argv[i], "--skip-menu") == 0)
        {
            config.skipMenu = true;
        }
    }

    return config;
}

void CommandLineHandler::ShowConfig(const GameConfig &config)
{
    TraceLog(LOG_INFO, "=== Game Configuration ===");
    TraceLog(LOG_INFO, "Resolution: %dx%d", config.width, config.height);
    TraceLog(LOG_INFO, "Fullscreen: %s", config.fullscreen ? "Yes" : "No");
    TraceLog(LOG_INFO, "Developer Mode: %s", config.developer ? "Yes" : "No");
    if (!config.mapPath.empty())
    {
        TraceLog(LOG_INFO, "Map: %s", config.mapPath.c_str());
    }
    TraceLog(LOG_INFO, "========================");
}
