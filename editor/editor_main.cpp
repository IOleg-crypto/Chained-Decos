#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "panels/console_panel.h"
#include "engine/core/log.h"

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    Log::SetLogCallback(ConsolePanel::AddLog);

    ApplicationSpecification spec;
    spec.Name = "Chained Editor";
    spec.CommandLineArgs = args;

    auto app = new Application(spec);
    app->PushLayer(new EditorLayer());
    return app;
}
} // namespace CHEngine
