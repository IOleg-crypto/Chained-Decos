#include "editor.h"
#include "engine/core/entry_point.h"

namespace CHEngine
{
Application *CreateApplication(int argc, char **argv)
{
    Application::Config config;
    config.Title = "Chained Editor";
    config.Width = 1600;
    config.Height = 900;
    config.Fullscreen = false;
    config.TargetFPS = 144;

    // Register project scripts so they appear in Inspector
    extern void RegisterProjectScripts();
    RegisterProjectScripts();

    return new Editor(config);
}
} // namespace CHEngine
