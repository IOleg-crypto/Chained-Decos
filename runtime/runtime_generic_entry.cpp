#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "engine/core/project_launcher.h"
#include "runtime/runtime_layer.h"
#include "scripting/scriptengine.h"
#include <filesystem>

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    auto details = ProjectLauncher::PrepareRuntime(args);
    
    details.Spec.InitScripting = []() { ScriptEngine::Get().Initialize(); };
    details.Spec.ShutdownScripting = []() { ScriptEngine::Get().Shutdown(); };

    // Create the application with orchestrated settings.
    auto app = new Application(details.Spec);
    
    // The runtime layer will handle the higher-level project logic.
    app->PushLayer(std::make_unique<RuntimeLayer>(details.ProjectPath.string()));
    
    return app;
}
} // namespace CHEngine
