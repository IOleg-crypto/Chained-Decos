#include "editor.h"
#include "editor_settings.h"
#include <engine/core/entry_point.h>

namespace CHEngine
{
Application *CreateApplication(int argc, char **argv)
{
    EditorSettings::Init();
    const auto &settings = EditorSettings::Get();

    Application::Config config;
    config.Title = "Chained Editor";
    config.Width = settings.WindowWidth;
    config.Height = settings.WindowHeight;
    config.Fullscreen = settings.Fullscreen;
    config.TargetFPS = settings.TargetFPS;
    // Register project scripts so they appear in Inspector
    extern void RegisterProjectScripts();
    RegisterProjectScripts();

    return new Editor(config);
}
} // namespace CHEngine
