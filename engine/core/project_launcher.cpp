#include "project_launcher.h"
#include "engine/scene/project.h"
#include "engine/core/log.h"
#include <filesystem>

namespace CHEngine
{
LaunchDetails ProjectLauncher::PrepareRuntime(const ApplicationCommandLineArgs& args)
{
    LaunchDetails details;
    details.Spec.CommandLineArgs = args;

    std::string projectPath = "";
    std::string appName = "Chained Runtime";
    int startupWidth = 1280;
    int startupHeight = 720;

    // 1. Basic CLI Parsing (Priority: CLI > Config)
    for (int i = 1; i < args.Count; ++i)
    {
        std::string arg = args.Args[i];
        if ((arg == "--project" || arg == "-p") && i + 1 < args.Count)
        {
            projectPath = args.Args[++i];
            // Remove quotes if present
            if (projectPath.size() >= 2 && projectPath.front() == '"' && projectPath.back() == '"')
                projectPath = projectPath.substr(1, projectPath.size() - 2);
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

    // 2. Discover Project File
    std::filesystem::path projectFile = projectPath;
    if (projectFile.empty())
    {
        std::filesystem::path exePath = std::filesystem::absolute(std::filesystem::path(args.Args[0]));
        projectFile = Project::Discover(exePath.parent_path(), appName);
    }

    // 3. Load Project Config for fallback/defaults
    if (!projectFile.empty())
    {
        if (auto project = Project::Load(projectFile))
        {
            const auto& config = project->GetConfig();
            
            // Apply config if not overridden by CLI
            if (startupWidth == 1280 && startupHeight == 720)
            {
                startupWidth = config.Window.Width;
                startupHeight = config.Window.Height;
            }

            if (appName == "Chained Runtime")
            {
                appName = config.Name;
            }

            details.Spec.VSync = config.Window.VSync;
            details.Spec.Fullscreen = config.Runtime.Fullscreen;
            details.Spec.Resizable = config.Window.Resizable;
            details.Spec.AppIcon = config.IconPath;

            details.ProjectPath = projectFile;
            details.Success = true;
        }
    }

    // 4. Finalize Specification
    details.Spec.Name = appName;
    details.Spec.WindowWidth = startupWidth;
    details.Spec.WindowHeight = startupHeight;

    return details;
}

LaunchDetails ProjectLauncher::PrepareEditor(const ApplicationCommandLineArgs& args)
{
    // Currently editor uses fixed defaults, but could be expanded here.
    LaunchDetails details;
    details.Spec.Name = "Chained Editor";
    details.Spec.WindowWidth = 0; // 0 usually means maximized in engine
    details.Spec.WindowHeight = 0;
    details.Spec.CommandLineArgs = args;
    details.Success = true;
    return details;
}
} // namespace CHEngine
