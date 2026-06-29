#include "layer.h"
#include "engine/app/entry_point.h"
#include "engine/core/platform.h"

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

    // Set engine root to the executable directory so AssetManager can find
    // resources/shaders, resources/icons, resources/font etc.
    spec.EngineRoot = Platform::GetExecutableDirectory();
    spec.WorkingDirectory = Platform::GetExecutableDirectory().string();

    auto* app = new Application(spec);

    app->PushLayer(std::make_unique<EditorLayer>(*app));

    return app;
}
} // namespace Chained
