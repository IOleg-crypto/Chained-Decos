#include "editor_layer.h"
#include "engine/core/entry_point.h"
#include "engine/core/project_launcher.h"
#include "panels/console_panel.h"
#include "engine/core/log.h"

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    Log::SetLogCallback(ConsolePanel::AddLog);

    auto details = ProjectLauncher::PrepareEditor(args);

    auto app = new Application(details.Spec);
    app->PushLayer(new EditorLayer());
    return app;
}
} // namespace CHEngine
