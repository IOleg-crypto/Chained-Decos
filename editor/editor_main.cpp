#include "editor_layer.h"
#include "engine/core/entry_point.h"
#include "engine/core/project_launcher.h"
#include "panels/console_panel.h"
#include "engine/core/log.h"
#include "scripting/scriptengine.h"

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    Log::SetLogCallback(ConsolePanel::AddLog);

    auto details = ProjectLauncher::PrepareEditor(args);

    details.Spec.InitScripting = []() { ScriptEngine::Init(); };
    details.Spec.ShutdownScripting = []() { ScriptEngine::Shutdown(); };

    auto app = new Application(details.Spec);
    app->PushLayer(new EditorLayer());
    return app;
}
} // namespace CHEngine
