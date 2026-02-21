#include "project_actions.h"
#include "editor_layer.h"
#include "engine/core/base.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/scene/project.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene_events.h"
#include "nfd.h"
#include <filesystem>
#include <format>

namespace CHEngine
{
void ProjectActions::New()
{
    // Simple default: close active project to show Project Browser
    Project::SetActive(nullptr);
}

void ProjectActions::New(const std::string& name, const std::string& path)
{
    Project::New();
    auto project = Project::GetActive();
    project->GetConfig().Name = name;
    project->GetConfig().ProjectDirectory = path;

    ProjectSerializer serializer(project);
    serializer.Serialize((std::filesystem::path(path) / (name + ".chproject")).string());
}

void ProjectActions::Open()
{
    nfdchar_t* outPath = NULL;
    nfdu8filteritem_t filterItem[1] = {{"Chained Project", "chproject"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
    if (result == NFD_OKAY)
    {
        Open(outPath);
        NFD_FreePath(outPath);
    }
}

void ProjectActions::Open(const std::filesystem::path& path)
{
    if (Project::Load(path))
    {
        EditorLayer::Get().SetLastProjectPath(path.string());
        ProjectOpenedEvent e(path.string());
        Application::Get().OnEvent(e);
    }
}

void ProjectActions::Save()
{
    auto project = Project::GetActive();
    ProjectSerializer serializer(project);
    serializer.Serialize((project->GetConfig().ProjectDirectory / (project->GetConfig().Name + ".chproject")).string());
}

static std::filesystem::path FindRuntimeExecutable(const std::string& projectName, const std::string& configStr)
{
    CH_PROFILE_FUNCTION();

    std::filesystem::path root;
#ifdef PROJECT_ROOT_DIR
    root = PROJECT_ROOT_DIR;
#else
    root = std::filesystem::current_path();
    while (root.has_parent_path() && !std::filesystem::exists(root / "CMakeLists.txt"))
    {
        root = root.parent_path();
    }
#endif

    if (!std::filesystem::exists(root))
    {
        CH_CORE_ERROR("FindRuntimeExecutable: Root path not found: {}", root.string());
        return {};
    }

    CH_CORE_INFO("Searching for runtime in: {}", root.string());

#ifdef CH_PLATFORM_WINDOWS
    std::string targetName = "ChainedRuntime.exe";
#else
    std::string targetName = "ChainedRuntime";
#endif

    // 1. Check directory of currently running editor (most reliable for portability)
    // We can't easily get the process handle here without platform code,
    // but we can check the current working directory bin folder
    std::filesystem::path currentBin = std::filesystem::current_path() / targetName;
    if (std::filesystem::exists(currentBin))
    {
        CH_CORE_INFO("FindRuntimeExecutable: Found in current directory: {}", currentBin.string());
        return currentBin;
    }

    // 2. Fast path: check common output locations including build presets
    std::vector<std::string> searchSubdirs = {"build/bin", "bin", "out/bin", "cmake-build-debug/bin",
                                              "cmake-build-release/bin"};

    // Add dynamic preset search build/*/bin
    if (std::filesystem::exists(root / "build"))
    {
        for (const auto& entry : std::filesystem::directory_iterator(root / "build"))
        {
            if (entry.is_directory())
            {
                std::filesystem::path p = entry.path() / "bin" / targetName;
                if (std::filesystem::exists(p))
                {
                    searchSubdirs.push_back("build/" + entry.path().filename().string() + "/bin");
                }
            }
        }
    }

    // Search collected paths
    for (const auto& sub : searchSubdirs)
    {
        std::filesystem::path p = root / sub / targetName;
        if (std::filesystem::exists(p))
        {
            CH_CORE_INFO("FindRuntimeExecutable: Path found at: {}", p.string());
            return p;
        }
    }

    // 3. Fallback: careful recursive search excluding noisy folders
    CH_CORE_INFO("FindRuntimeExecutable: Fast path failed, starting scoped recursive search...");
    try
    {
        for (auto it = std::filesystem::recursive_directory_iterator(root);
             it != std::filesystem::recursive_directory_iterator(); ++it)
        {
            const auto& entry = *it;
            auto filename = entry.path().filename().string();
            auto pathStr = entry.path().string();

            // Skip noisy/irrelevant directories
            if (entry.is_directory())
            {
                if (filename == ".git" || filename == ".cache" || filename == ".idea" || filename == "include" ||
                    filename == "engine")
                {
                    it.disable_recursion_pending();
                    continue;
                }
            }

            if (entry.is_regular_file() && filename == targetName)
            {
                CH_CORE_INFO("FindRuntimeExecutable: Deep search found at: {}", pathStr);
                return entry.path();
            }
        }
    } catch (const std::exception& e)
    {
        CH_CORE_WARN("FindRuntimeExecutable: Deep search error: {}", e.what());
    }

    CH_CORE_ERROR("FindRuntimeExecutable: Failed to find '{}' in {}", targetName, root.string());
    return {};
}

static std::string ResolveLaunchVariables(std::string str)
{
    CH_PROFILE_FUNCTION();

    auto project = Project::GetActive();
    if (!project)
    {
        return str;
    }

    std::filesystem::path root;
#ifdef PROJECT_ROOT_DIR
    root = PROJECT_ROOT_DIR;
#else
    root = std::filesystem::current_path();
    while (root.has_parent_path() && !std::filesystem::exists(root / "CMakeLists.txt"))
    {
        root = root.parent_path();
    }
#endif

    std::filesystem::path projectFile = project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
    std::string projectPathStr = std::filesystem::absolute(projectFile).string();

    // 1. Resolve ${ROOT}
    size_t pos = 0;
    while ((pos = str.find("${ROOT}")) != std::string::npos)
    {
        str.replace(pos, 7, std::filesystem::absolute(root).string());
    }

    // 2. Resolve ${PROJECT_FILE}
    while ((pos = str.find("${PROJECT_FILE}")) != std::string::npos)
    {
        str.replace(pos, 15, projectPathStr);
    }

    // 3. Resolve ${BUILD} - Intelligent discovery
    if (str.find("${BUILD}") != std::string::npos)
    {
        std::string configStr = (project->GetConfig().BuildConfig == Configuration::Release) ? "Release" : "Debug";
        std::filesystem::path exePath = FindRuntimeExecutable(project->GetConfig().Name, configStr);
        std::filesystem::path buildPath = exePath.parent_path();

        if (buildPath.empty())
        {
            // Last ditch effort if FindRuntimeExecutable failed
            std::vector<std::string> searchSubdirs = {"build/bin", "bin", "out/bin", "cmake-build-debug/bin",
                                                      "cmake-build-release/bin"};

            for (const auto& sub : searchSubdirs)
            {
                if (std::filesystem::exists(root / sub))
                {
                    buildPath = root / sub;
                    break;
                }
            }
        }

        while ((pos = str.find("${BUILD}")) != std::string::npos)
        {
            str.replace(pos, 8, std::filesystem::absolute(buildPath).string());
        }
    }

    return str;
}

void ProjectActions::LaunchStandalone()
{
    CH_PROFILE_FUNCTION();
    Save();

    auto project = Project::GetActive();
    if (!project)
    {
        return;
    }

    auto& config = project->GetConfig();

    std::string runtimePath;
    std::string arguments;

    if (!config.LaunchProfiles.empty() && config.ActiveLaunchProfileIndex >= 0 &&
        config.ActiveLaunchProfileIndex < (int)config.LaunchProfiles.size())
    {
        const auto& profile = config.LaunchProfiles[config.ActiveLaunchProfileIndex];
        runtimePath = ResolveLaunchVariables(profile.BinaryPath);
        arguments = ResolveLaunchVariables(profile.Arguments);

        if (profile.UseDefaultArgs)
        {
            std::filesystem::path projectFile =
                project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
            arguments += std::format(" \"{}\"", std::filesystem::absolute(projectFile).string());
        }
    }
    else
    {
        // Fallback to old heuristic if no profiles
        CH_CORE_WARN("LaunchStandalone: No active launch profile. Falling back to heuristic search.");
        std::string configStr = (config.BuildConfig == Configuration::Release) ? "Release" : "Debug";
        runtimePath = FindRuntimeExecutable(config.Name, configStr).string();

        std::filesystem::path projectFile = project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
        arguments = std::format("\"{}\"", std::filesystem::absolute(projectFile).string());
    }

    if (runtimePath.empty() || !std::filesystem::exists(runtimePath))
    {
        CH_CORE_WARN("LaunchStandalone: Profile binary not found at '{}'. Searching heuristic...", runtimePath);
        std::string configStr = (config.BuildConfig == Configuration::Release) ? "Release" : "Debug";
        runtimePath = FindRuntimeExecutable(config.Name, configStr).string();

        if (runtimePath.empty())
        {
            CH_CORE_ERROR("LaunchStandalone: Runtime executable not found!");
            return;
        }
    }

#ifdef CH_PLATFORM_WINDOWS
    std::string command = std::format("start \"\" \"{}\" {}", runtimePath, arguments);
#else
    std::string command = std::format("\"{}\" {} &", runtimePath, arguments);
#endif
    CH_CORE_INFO("Launching Standalone: {}", command);

    system(command.c_str());
}
} // namespace CHEngine
