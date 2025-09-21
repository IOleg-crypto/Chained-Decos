#ifndef COMMAND_LINE_HANDLER_H
#define COMMAND_LINE_HANDLER_H

#include <string>
#include <vector>

struct GameConfig
{
    int width = 1280;
    int height = 720;
    bool fullscreen = false;
    bool vsync = true;
    int targetFPS = 60;
    bool noBorder = false;
    std::string map = "";
    bool dedicated = false;
    bool developer = false;
    int heapSize = 256; // MB
};

class CommandLineHandler
{
public:
    static GameConfig ParseArguments(int argc, char* argv[]);
    static void ShowHelp();
    static void ShowConfig(const GameConfig& config);
    static void ApplyConfigToEngine(const GameConfig& config);
};

#endif // COMMAND_LINE_HANDLER_H