#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "runtime/runtime_application.h"
#include <filesystem>


namespace CHEngine
{
    extern void RegisterGameScripts();

    Application *CreateApplication(ApplicationCommandLineArgs args)
    {
        // 1. Setup Project-specific configuration (Title, Icon, etc.)
        ApplicationSpecification spec;
        spec.Name = "Chained Decos";
        spec.CommandLineArgs = args;

        RegisterGameScripts();

        // 2. Project Selection
        std::string projectPath = ""; // RuntimeApplication will auto-discover if empty

        // Handle CLI overrides
        for (int i = 1; i < args.Count; ++i)
        {
            std::string arg = args.Args[i];
            if (arg == "--project" && i + 1 < args.Count)
            {
                projectPath = args.Args[++i];
            }
            else if (arg == "--scene" && i + 1 < args.Count)
            {
                // Note: Scene overrides should probably be handled by the layer or project loader
            }
            else if (i == 1 && arg[0] != '-')
            {
                projectPath = arg;
            }
        }

        // 3. Create the generic runtime host
        // Automated discovery will happen inside RuntimeApplication
        return new RuntimeApplication(spec, projectPath);
    }
} // namespace CHEngine
