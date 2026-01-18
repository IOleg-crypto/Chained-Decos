#include "editor.h"
#include "editor_settings.h"
#include <engine/core/entry_point.h>

namespace CHEngine
{
Application *CreateApplication()
{
    EditorSettings::Init();
    const auto &settings = EditorSettings::Get();

    Application::Config config;
    config.Title = "Chained Editor";
    config.Width = settings.WindowWidth;
    config.Height = settings.WindowHeight;
    config.Fullscreen = settings.Fullscreen;
    config.TargetFPS = settings.TargetFPS;

    return new Editor(config);
}
} // namespace CHEngine
