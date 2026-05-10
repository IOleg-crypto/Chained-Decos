#include "editor_layer.h"
#include "engine/core/entry_point.h"

namespace CHEngine
{
extern void RegisterGameComponents();

Application* CreateApplication(ApplicationCommandLineArgs args)
{
    ApplicationSpecification spec;
    spec.Name = "ChainedEditor";
    spec.CommandLineArgs = args;

    // Default editor window settings
    spec.WindowWidth = 1600;
    spec.WindowHeight = 900;
    spec.Headless = false;

    auto* app = new Application(spec);

    // Register game components AFTER Application is created,
    // because ComponentSerializer is initialized inside Application's constructor.
    RegisterGameComponents();

    app->PushLayer(std::make_unique<EditorLayer>());

    return app;
}
} // namespace CHEngine

int main(int argc, char** argv)
{
    return CHEngine::RunEntryPoint(argc, argv);
}
