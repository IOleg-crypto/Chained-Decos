#include "runtime_application.h"
#include <engine/core/entry_point.h>
#include <string>
#include <vector>

namespace CHEngine
{
// We need to capture argc/argv if we want to pass them to CreateApplication
// But since entry_point.h calls CreateApplication() without args, we can use a global or
// a more complex mechanism. For now, let's keep it simple and maybe entry_point.h should be updated
// or we can use __argc / __argv on Windows.
extern void RegisterGameScripts();

Application *CreateApplication()
{
    RegisterGameScripts();
    Application::Config config;
    // config.Title = "Chained e";
    config.Width = 1280;
    config.Height = 720;

    std::string projectPath = "";
#ifdef _WIN32
    // Quick hack for Windows to get command line arguments in a library/exe without passing them
    if (__argc > 1)
    {
        projectPath = __argv[1];
    }
#endif

    return new RuntimeApplication(config, projectPath);
}
} // namespace CHEngine
