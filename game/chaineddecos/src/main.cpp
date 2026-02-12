#include "engine/core/entry_point.h"
#include "runtime/runtime_application.h"

// Declaration for the function in game_module.cpp
void RegisterGameScripts(CHEngine::Scene* scene);

namespace CHEngine
{
    Application* CreateApplication(ApplicationCommandLineArgs args)
    {
        ApplicationSpecification spec;
        spec.Name = "Chained Decos";
        spec.CommandLineArgs = args;

        // Pass empty string to trigger auto-discovery of .chproject
        // Pass the static registration function
        return new RuntimeApplication(spec, "", RegisterGameScripts);
    }
}
