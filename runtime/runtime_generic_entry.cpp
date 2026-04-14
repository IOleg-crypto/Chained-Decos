#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "engine/core/project_launcher.h"
#include "runtime/runtime_layer.h"
#include <filesystem>

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    auto details = ProjectLauncher::PrepareRuntime(args);
    
    // Create the application with orchestrated settings.
    auto app = new Application(details.Spec);
    
    // The runtime layer will handle the higher-level project logic.
    app->PushLayer(new RuntimeLayer(details.ProjectPath.string()));
    
    return app;
}
} // namespace CHEngine
