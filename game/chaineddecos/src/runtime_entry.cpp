#include "engine/core/application.h"
#include "runtime/runtime_application.h"
#include <engine/core/entry_point.h>

namespace CHEngine
{
extern void RegisterProjectScripts();

Application *CreateApplication(int argc, char **argv)
{
    // 1. Setup Project-specific configuration (Title, Icon, etc.)
    Application::Config config;
    config.Title = "Chained Decos";
    config.Width = 1280;
    config.Height = 720;
    config.Argc = argc;
    config.Argv = argv;

    RegisterProjectScripts();

    // 3. Resolve project path (specific to this standalone build)
    std::string projectPath = PROJECT_ROOT_DIR "/game/chaineddecos/ChainedDecos.chproject";

    // Handle CLI overrides
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--project" && i + 1 < argc)
            projectPath = argv[++i];
        else if (arg == "--scene" && i + 1 < argc)
            Application::SetStartupScene(argv[++i]);
        else if (i == 1 && arg[0] != '-')
            projectPath = arg;
    }

    // 4. Create the generic runtime host with this project configuration
    return new RuntimeApplication(config, projectPath);
}
} // namespace CHEngine
