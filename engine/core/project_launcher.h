#ifndef CH_PROJECT_LAUNCHER_H
#define CH_PROJECT_LAUNCHER_H

#include "engine/core/application.h"
#include <filesystem>
#include <string>

namespace CHEngine
{
// Result of a project-aware launch preparation.
struct LaunchDetails
{
    ApplicationSpecification Spec;
    std::filesystem::path ProjectPath;
    bool Success = false;
};

// Utility for automating the discovery and configuration of a project
// before the Application is truly initialized.
class ProjectLauncher
{
public:
    // Prepares common initialization for a standalone runtime.
    // Handles CLI arguments and project discovery.
    static LaunchDetails PrepareRuntime(const ApplicationCommandLineArgs& args);

    // Prepares initialization for the editor.
    static LaunchDetails PrepareEditor(const ApplicationCommandLineArgs& args);
};
} // namespace CHEngine

#endif // CH_PROJECT_LAUNCHER_H
