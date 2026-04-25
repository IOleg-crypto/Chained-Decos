#include "engine/core/application.h"
#include "engine/core/entry_point.h"
#include "engine/core/project_launcher.h"
#include "engine/core/service_locator.h"
#include "engine/core/assets/asset_manager.h"
#include "runtime/runtime_layer.h"
#include "scripting/scriptengine.h"
#include <filesystem>

namespace CHEngine
{
Application* CreateApplication(ApplicationCommandLineArgs args)
{
    // Early register AssetManager because ProjectLauncher::PrepareRuntime (via Project::Load)
    // might need to access assets (like Environment) before the Application constructor runs.
    if (!ServiceLocator::Has<AssetManager>())
    {
        ServiceLocator::Register<AssetManager>(std::make_shared<AssetManager>());
    }

    auto details = ProjectLauncher::PrepareRuntime(args);
    
    // Enable scripting in the specification (default is true, but we make it explicit here)
    details.Spec.EnableScripting = true;

    // Create the application with orchestrated settings.
    auto app = new Application(details.Spec);
    
    // The runtime layer will handle the higher-level project logic.
    app->PushLayer(std::make_unique<RuntimeLayer>(details.ProjectPath.string()));
    
    return app;
}
} // namespace CHEngine
