#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "runtime/runtime_layer.h"

// Declaration for the function in game_module.cpp
void RegisterGameScripts(CHEngine::Scene* scene);

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    ApplicationSpecification spec;
    spec.Name = "Chained Decos";
    spec.CommandLineArgs = args;

    // Pass empty string to trigger auto-discovery of .chproject
    // Pass the static registration function
    Application* app = new Application(spec);
    app->PushLayer(new RuntimeLayer("", RegisterGameScripts));
    return app;
}
} // namespace CHEngine
