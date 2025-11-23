#pragma once

#include <string>

// Game configuration from command line
struct GameConfig
{
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool developer = false;
    std::string mapPath = "";
    bool skipMenu = false;
};

// Command line handler
class CommandLineHandler
{
public:
    static GameConfig ParseArguments(int argc, char *argv[]);
    static void ShowConfig(const GameConfig &config);
};
