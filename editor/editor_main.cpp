#include "editor.h"
#include "engine/core/entry_point.h"

namespace CHEngine
{
Application *CreateApplication(ApplicationCommandLineArgs args)
{
    ApplicationSpecification spec;
    spec.Name = "Chained Editor";
    spec.CommandLineArgs = args;

    // Register project scripts so they appear in Inspector
    extern void RegisterGameScripts();
    RegisterGameScripts();

    return new Editor(spec);
}
} // namespace CHEngine
