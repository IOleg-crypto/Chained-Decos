#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "engine/scene/project.h"
#include "runtime/runtime_layer.h"
#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    ApplicationSpecification spec;
    spec.CommandLineArgs = args;

    // 1. Initial Project Path (can be empty for auto-discovery)
    std::string projectPath = "";
    std::string appName = "Chained Runtime";
    int startupWidth = 1280;
    int startupHeight = 720;

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
        else if (arg == "--width" && i + 1 < args.Count)
        {
            startupWidth = std::stoi(args.Args[++i]);
        }
        else if (arg == "--height" && i + 1 < args.Count)
        {
            startupHeight = std::stoi(args.Args[++i]);
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

    // If width/height were not overridden via CLI, try preloading startup size from project config.
    if (startupWidth == 1280 && startupHeight == 720)
    {
        std::filesystem::path projectFile = projectPath;
        if (projectFile.empty())
        {
            std::filesystem::path exePath = std::filesystem::absolute(std::filesystem::path(args.Args[0]));
            projectFile = Project::Discover(exePath.parent_path(), appName);
        }

        if (!projectFile.empty() && std::filesystem::exists(projectFile))
        {
            try
            {
                YAML::Node root = YAML::LoadFile(projectFile.string());
                YAML::Node projectNode = root["Project"];
                YAML::Node windowNode = projectNode ? projectNode["Window"] : YAML::Node();
                if (windowNode)
                {
                    if (windowNode["Width"])
                    {
                        startupWidth = windowNode["Width"].as<int>();
                    }
                    if (windowNode["Height"])
                    {
                        startupHeight = windowNode["Height"].as<int>();
                    }
                }
            }
            catch (...)
            {
                // Keep defaults on parse errors; runtime layer will still apply config later.
            }
        }
    }

    spec.Name = appName;
    spec.WindowWidth = startupWidth;
    spec.WindowHeight = startupHeight;

    auto app = new Application(spec);
    app->PushLayer(new RuntimeLayer(projectPath));
    return app;
}
} // namespace CHEngine
