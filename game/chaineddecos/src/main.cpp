#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "runtime/runtime_layer.h"
#include "scripting/scriptengine.h"

namespace
{
constexpr const char* kProjectGame = "ChainedDecos";
}

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    ApplicationSpecification spec;
    spec.Name = kProjectGame;
    spec.CommandLineArgs = args;

    spec.InitScripting = []() { ScriptEngine::Init(); };
    spec.ShutdownScripting = []() { ScriptEngine::Shutdown(); };

    Application* app = new Application(spec);
    app->PushLayer(std::make_unique<RuntimeLayer>(""));
    return app;
}
} // namespace CHEngine
