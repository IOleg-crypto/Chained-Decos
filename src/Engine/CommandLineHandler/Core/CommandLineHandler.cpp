#include "CommandLineHandler.h"
#include <iostream>
#include <string>
#include <algorithm>
#include <raylib.h>

GameConfig CommandLineHandler::ParseArguments(int argc, char* argv[])
{
    GameConfig config;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-width" || arg == "-w")
        {
            if (i + 1 < argc)
            {
                config.width = std::atoi(argv[++i]);
            }
        }
        else if (arg == "-height" || arg == "-h")
        {
            if (i + 1 < argc)
            {
                config.height = std::atoi(argv[++i]);
            }
        }
        else if (arg == "-fullscreen" || arg == "-f")
        {
            config.fullscreen = true;
        }
        else if (arg == "-windowed" || arg == "-window")
        {
            config.fullscreen = false;
        }
        else if (arg == "-noborder" || arg == "-borderless")
        {
            config.noBorder = true;
        }
        else if (arg == "-novsync")
        {
            config.vsync = false;
        }
        else if (arg == "-fps")
        {
            if (i + 1 < argc)
            {
                config.targetFPS = std::atoi(argv[++i]);
            }
        }
        else if (arg == "-map")
        {
            if (i + 1 < argc)
            {
                config.map = argv[++i];
            }
        }
        else if (arg == "-dedicated")
        {
            config.dedicated = true;
        }
        else if (arg == "-dev" || arg == "-developer")
        {
            config.developer = true;
        }
        else if (arg == "-heapsize")
        {
            if (i + 1 < argc)
            {
                config.heapSize = std::atoi(argv[++i]);
            }
        }
        else if (arg == "-help" || arg == "-?")
        {
            ShowHelp();
            // Don't exit here, let main handle it
        }
    }

    return config;
}

void CommandLineHandler::ShowHelp()
{
    std::cout << "Chained Decos - Command Line Options:" << std::endl;
    std::cout << "  -width <width>        Set window width" << std::endl;
    std::cout << "  -height <height>      Set window height" << std::endl;
    std::cout << "  -fullscreen           Start in fullscreen mode" << std::endl;
    std::cout << "  -windowed             Start in windowed mode" << std::endl;
    std::cout << "  -noborder             Start in borderless window mode" << std::endl;
    std::cout << "  -novsync              Disable VSync" << std::endl;
    std::cout << "  -fps <fps>            Set target FPS (0 for unlimited)" << std::endl;
    std::cout << "  -map <mapname>        Load specific map" << std::endl;
    std::cout << "  -dedicated            Run as dedicated server" << std::endl;
    std::cout << "  -dev                  Enable developer mode" << std::endl;
    std::cout << "  -heapsize <MB>        Set heap size in MB" << std::endl;
    std::cout << "  -help                 Show this help" << std::endl;
}

void CommandLineHandler::ShowConfig(const GameConfig& config)
{
    std::cout << "Game Configuration:" << std::endl;
    std::cout << "  Resolution: " << config.width << "x" << config.height << std::endl;
    std::cout << "  Fullscreen: " << (config.fullscreen ? "Yes" : "No") << std::endl;
    std::cout << "  VSync: " << (config.vsync ? "Enabled" : "Disabled") << std::endl;
    std::cout << "  Target FPS: " << (config.targetFPS == 0 ? "Unlimited" : std::to_string(config.targetFPS)) << std::endl;
    std::cout << "  Borderless: " << (config.noBorder ? "Yes" : "No") << std::endl;
    std::cout << "  Map: " << (config.map.empty() ? "Default" : config.map) << std::endl;
    std::cout << "  Dedicated: " << (config.dedicated ? "Yes" : "No") << std::endl;
    std::cout << "  Developer: " << (config.developer ? "Yes" : "No") << std::endl;
    std::cout << "  Heap Size: " << config.heapSize << " MB" << std::endl;
}

void CommandLineHandler::ApplyConfigToEngine(const GameConfig& config)
{
    // Apply window settings
    if (config.fullscreen)
    {
        if (config.noBorder)
        {
            SetWindowState(FLAG_FULLSCREEN_MODE | FLAG_WINDOW_UNDECORATED);
        }
        else
        {
            SetWindowState(FLAG_FULLSCREEN_MODE);
        }
    }
    else if (config.noBorder)
    {
        SetWindowState(FLAG_WINDOW_UNDECORATED);
    }

    // Apply VSync setting
    if (config.vsync)
    {
        SetWindowState(FLAG_VSYNC_HINT);
    }
    else
    {
        ClearWindowState(FLAG_VSYNC_HINT);
    }

    // Set target FPS
    SetTargetFPS(config.targetFPS);
}