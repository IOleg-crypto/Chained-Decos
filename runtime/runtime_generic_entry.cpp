#include "engine/app/application.h"
#include "engine/project/project.h"
#include "engine/app/entry_point.h"
#include "runtime/runtime_layer.h"


namespace Chained
{
// Forward declare the game's registration function
// This lives in the ChainedDecos static library
extern void RegisterGameComponents();

Application* CreateApplication(ApplicationCommandLineArgs args)
{
    // Ensure game components are registered
    RegisterGameComponents();

    ApplicationSpecification spec;
    spec.Name = "ChainedRuntime";
    spec.CommandLineArgs = args;
    spec.WorkingDirectory = Application::GetExecutableDirectory().string();

    // Runtime defaults (can be overridden by project config when supported)
    spec.WindowWidth = 0;
    spec.WindowHeight = 0;
    spec.Headless = false;
    spec.EnableScripting = true;

    // First CLI argument is the project file path (passed by the editor).
    // Format: ChainedRuntime.exe "path/to/project.chproject" --scene "path/to/scene.chscene"
    std::filesystem::path projectPath;
    for (int i = 1; i < args.Count; ++i)
    {
        std::string arg = args.Args[i];
        if (arg == "--scene" || arg == "--width" || arg == "--height" || arg == "--fullscreen" ||
            arg == "--windowed" || arg == "--vsync")
        {
            ++i; // skip value
            continue;
        }

        // First non-flag argument is the project file
        std::filesystem::path candidate(arg);
        if (candidate.extension() == ".chproject")
        {
            projectPath = candidate;
            break;
        }
    }

    // Fallback: look for a .chproject next to the executable
    if (projectPath.empty() || !std::filesystem::exists(projectPath))
    {
        projectPath = Application::GetExecutableDirectory() / (spec.Name + ".chproject");
    }

    CH_CORE_INFO("RuntimeEntry: Project path resolved to: {}", projectPath.string());

    // Change working directory to the project root so that engine-relative paths
    // (e.g. "engine/resources/config/shaders.yaml") resolve correctly.
    // The .chproject sits inside a subfolder of the project root (e.g. game/chaineddecos/),
    // so go up until we find a CMakeLists.txt, or just go two levels up.
    std::filesystem::path projectRoot = projectPath.parent_path();
    while (projectRoot.has_parent_path())
    {
        if (std::filesystem::exists(projectRoot / "CMakeLists.txt"))
        {
            break;
        }
        // Stop if we've reached the filesystem root
        if (projectRoot == projectRoot.parent_path())
        {
            break;
        }
        projectRoot = projectRoot.parent_path();
    }

    if (std::filesystem::exists(projectRoot / "CMakeLists.txt"))
    {
        std::error_code ec;
        std::filesystem::current_path(projectRoot, ec);
        if (ec)
        {
            CH_CORE_WARN("RuntimeEntry: Failed to set working directory to {}: {}", projectRoot.string(), ec.message());
        }
        else
        {
            CH_CORE_INFO("RuntimeEntry: Working directory set to: {}", projectRoot.string());
        }
    }
    else
    {
        CH_CORE_WARN("RuntimeEntry: Could not locate project root (no CMakeLists.txt found). "
                     "Engine resources may fail to load.");
    }

    auto* app = new Application(spec);
    app->PushLayer(std::make_unique<RuntimeLayer>(projectPath.string()));

    return app;
}
} // namespace Chained
