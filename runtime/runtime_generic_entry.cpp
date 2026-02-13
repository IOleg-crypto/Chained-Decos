#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "runtime/runtime_application.h"
#include <filesystem>


namespace CHEngine
{
    // No manual RegisterGameScripts here! It's loaded dynamically.

    Application *CreateApplication(ApplicationCommandLineArgs args)
    {
        ApplicationSpecification spec;
        spec.CommandLineArgs = args;

        // 1. Initial Project Path (can be empty for auto-discovery)
        std::string projectPath = ""; 
        std::string appName = "Chained Runtime";

        // 2. Minimal CLI parsing for bootstrapping
        // (Full parsing happens inside RuntimeApplication::InitProject)
        for (int i = 1; i < args.Count; ++i)
        {
            std::string arg = args.Args[i];
            if ((arg == "--project" || arg == "-p") && i + 1 < args.Count)
            {
                projectPath = args.Args[++i];
                if (projectPath.size() >= 2 && projectPath.front() == '"' && projectPath.back() == '"') {
                    projectPath = projectPath.substr(1, projectPath.size() - 2);
                }
            }
            else if (arg == "--name" && i + 1 < args.Count)
            {
                appName = args.Args[++i];
            }
            else if (i == 1 && arg[0] != '-')
            {
                projectPath = arg;
            }
        }
        
        // 3. Post-discovery hint for App Name
        if (!projectPath.empty() && appName == "Chained Runtime")
        {
            std::filesystem::path p = projectPath;
            appName = p.stem().string();
        }

        spec.Name = appName;

        // 3. Create the generic runtime host
        return new RuntimeApplication(spec, projectPath);
    }
} // namespace CHEngine
