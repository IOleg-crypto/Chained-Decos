#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "runtime/runtime_layer.h"
#include <filesystem>

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    ApplicationSpecification spec;
    spec.CommandLineArgs = args;

    // 1. Initial Project Path (can be empty for auto-discovery)
    std::string projectPath = "";
    std::string appName = "Chained Runtime";

    for (int i = 1; i < args.Count; ++i)
    {
        std::string arg = args.Args[i];
        if ((arg == "--project" || arg == "-p") && i + 1 < args.Count)
        {
            projectPath = args.Args[++i];
            if (projectPath.size() >= 2 && projectPath.front() == '"' && projectPath.back() == '"')
            {
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

    if (!projectPath.empty() && appName == "Chained Runtime")
    {
        std::filesystem::path p = projectPath;
        appName = p.stem().string();
    }

    spec.Name = appName;

    auto app = new Application(spec);
    app->PushLayer(new RuntimeLayer(projectPath));
    return app;
}
} // namespace CHEngine
