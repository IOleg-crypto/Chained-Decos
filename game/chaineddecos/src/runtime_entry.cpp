#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "runtime/runtime_application.h"
#include <filesystem>


namespace CHEngine
{
    extern void RegisterGameScripts();

    Application *CreateApplication(int argc, char **argv)
    {
        // 1. Setup Project-specific configuration (Title, Icon, etc.)
        Application::Config config;
        config.Title = "Chained Decos";
        config.Width = 1280;
        config.Height = 720;
        config.Argc = argc;
        config.Argv = argv;

        RegisterGameScripts();

        // 2. Project Selection
        std::string projectPath = ""; // RuntimeApplication will auto-discover if empty

        // Handle CLI overrides
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--project" && i + 1 < argc)
            {
                projectPath = argv[++i];
            }
            else if (arg == "--scene" && i + 1 < argc)
            {
                config.StartScene = argv[++i];
            }
            else if (i == 1 && arg[0] != '-')
            {
                projectPath = arg;
            }
        }

        // 3. Create the generic runtime host
        // Automated discovery will happen inside RuntimeApplication
        return new RuntimeApplication(config, projectPath);
    }
} // namespace CHEngine
