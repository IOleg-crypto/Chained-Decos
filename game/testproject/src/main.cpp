#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "runtime/runtime_layer.h"

namespace
{
constexpr const char* kProjectGame = "testproject";
}

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    ApplicationSpecification spec;
    spec.Name = kProjectGame;
    spec.CommandLineArgs = args;

    Application* app = new Application(spec);
    app->PushLayer(new RuntimeLayer(""));
    return app;
}
} // namespace CHEngine