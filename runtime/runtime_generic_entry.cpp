#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "engine/project/project.h"
#include "runtime/runtime_layer.h"

namespace Chained
{
    // Forward declare the game's registration function
    // This lives in the ChainedDecos static library
    extern void RegisterGameComponents();

Application* CreateApplication(ApplicationCommandLineArgs args)
{
    // Ensure game components are registered
    RegisterGameComponents();

    ApplicationSpecification spec;
    spec.Name = "ChainedRuntime";
    spec.CommandLineArgs = args;
    spec.WorkingDirectory = Application::GetExecutableDirectory().string();

    // Runtime defaults (can be overridden by project config when supported)
    spec.WindowWidth = 1280;
    spec.WindowHeight = 720;
    spec.Headless = false;
    spec.EnableScripting = true;

    std::filesystem::path projectPath = Application::GetExecutableDirectory() / (spec.Name + ".chproject");

    auto* app = new Application(spec);
    app->PushLayer(std::make_unique<RuntimeLayer>(projectPath.string()));

    return app;
}
} // namespace Chained

