#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "runtime/runtime_layer.h"

#include "engine/core/game_entry_point.h"

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    ApplicationSpecification spec;
    spec.Name = "Chained Decos";
    spec.CommandLineArgs = args;

    // Register scripts one time into Global Registry
    RegisterGameScripts();

    Application* app = new Application(spec);
    app->PushLayer(new RuntimeLayer(""));
    return app;
}
} // namespace CHEngine
