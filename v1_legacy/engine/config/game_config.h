#ifndef CD_ENGINE_CONFIG_GAME_CONFIG_H
#define CD_ENGINE_CONFIG_GAME_CONFIG_H

#include <string>

struct GameConfig
{
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool developer = false;
    std::string mapPath = "";
    bool skipMenu = false;
};

class CommandLineHandler
{
public:
    static GameConfig ParseArguments(int argc, char *argv[]);
    static void ShowConfig(const GameConfig &config);
};

#endif
