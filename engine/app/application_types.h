#ifndef CH_APPLICATION_TYPES_H
#define CH_APPLICATION_TYPES_H

#include <string>

namespace Chained
{
struct ApplicationCommandLineArgs
{
    int Count = 0;
    char** Args = nullptr;
};

struct ApplicationSpecification
{
    std::string Name;
    std::string WorkingDirectory;
    // Absolute path to the engine resources root (shaders, icons, default assets).
    // Set this in CreateApplication() to the directory that contains 'engine/resources/'.
    // For development builds: repo root. For distribution: directory next to the executable.
    std::filesystem::path EngineRoot;
    int WindowWidth = 1280;
    int WindowHeight = 720;
    bool VSync = true;
    bool Fullscreen = false;
    bool Resizable = true;
    std::string AppIcon;

    ApplicationCommandLineArgs CommandLineArgs;
    bool Headless = false;
    bool EnableScripting = true;
    std::string ImGuiConfigurationPath;
    
    ApplicationSpecification() : Name("Chained Application"), ImGuiConfigurationPath("imgui.ini") {}
};
}

#endif // CH_APPLICATION_TYPES_H
