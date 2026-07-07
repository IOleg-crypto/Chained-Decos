#include "engine/app/entry_point.h"
#include "engine/core/platform.h"
#include "layer.h"

namespace Chained
{
// Forward declare the game's registration function
// This lives in the ChainedDecos static library
extern void RegisterGameComponents();

Application* CreateApplication(ApplicationCommandLineArgs args)
{
    RegisterGameComponents();

    ApplicationSpecification spec;
#if CH_PLATFORM_BACKEND_SDL3
    spec.Name = "ChainedEditor | Backend: SDL3";
#else
    spec.Name = "ChainedEditor | Backend: GLFW";
#endif
    spec.CommandLineArgs = args;

    // Default editor window settings
    spec.WindowWidth = 1600;
    spec.WindowHeight = 900;
    spec.Headless = false;
    spec.EnableScripting = true;
    spec.Fullscreen = false;

    // Set engine root to the executable directory so AssetManager can find
    // resources/shaders, resources/icons, resources/font etc.
    spec.EngineRoot = Platform::GetExecutableDirectory();
    spec.WorkingDirectory = Platform::GetExecutableDirectory().string();

    auto* app = new Application(spec);

    app->PushLayer(std::make_unique<EditorLayer>());

    return app;
}
} // namespace Chained
