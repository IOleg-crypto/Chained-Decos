#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "runtime/runtime_application.h"
#include <filesystem>


namespace CHEngine
{
    // No manual RegisterGameScripts here! It's loaded dynamically.

    Application *CreateApplication(ApplicationCommandLineArgs args)
    {
        // 1. Setup Project-specific configuration (Title, Icon, etc.)
        // These could eventually be loaded from a config file before App creation
        ApplicationSpecification spec;
        spec.Name = "Chained Runtime";
        spec.CommandLineArgs = args;

        // 2. Project Selection
        std::string projectPath = ""; 

        // Handle CLI overrides
        for (int i = 1; i < args.Count; ++i)
        {
            std::string arg = args.Args[i];
            if (arg == "--project" && i + 1 < args.Count)
            {
                projectPath = args.Args[++i];
                // Strip quotes if present
                if (projectPath.size() >= 2 && projectPath.front() == '"' && projectPath.back() == '"') {
                    projectPath = projectPath.substr(1, projectPath.size() - 2);
                }
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
