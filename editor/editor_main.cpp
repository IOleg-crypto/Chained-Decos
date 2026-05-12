#include "editor_layer.h"
#include "engine/core/entry_point.h"

namespace CHEngine
{
    // Forward declare the game's registration function
    // This lives in the ChainedDecos static library
    extern void RegisterGameComponents();

Application* CreateApplication(ApplicationCommandLineArgs args)
{
    // Ensure game components are registered
    RegisterGameComponents();

    ApplicationSpecification spec;
    spec.Name = "ChainedEditor";
    spec.CommandLineArgs = args;

    // Default editor window settings
    spec.WindowWidth = 1600;
    spec.WindowHeight = 900;
    spec.Headless = false;

    auto* app = new Application(spec);

    app->PushLayer(std::make_unique<EditorLayer>());

    return app;
}
} // namespace CHEngine

int main(int argc, char** argv)
{
    return CHEngine::RunEntryPoint(argc, argv);
}
