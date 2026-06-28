#include "editor_layer.h"
#include "engine/app/entry_point.h"

namespace Chained
{
// Forward declare the game's registration function
// This lives in the ChainedDecos static library
extern void RegisterGameComponents();

Application* CreateApplication(ApplicationCommandLineArgs args)
{
    RegisterGameComponents();

    ApplicationSpecification spec;
    spec.Name = "ChainedEditor";
    spec.CommandLineArgs = args;

    // Default editor window settings
    spec.WindowWidth = 1600;
    spec.WindowHeight = 900;
    spec.Headless = false;
    spec.EnableScripting = true;

    auto* app = new Application(spec);

    app->PushLayer(std::make_unique<EditorLayer>(*app));

    return app;
}
} // namespace Chained
