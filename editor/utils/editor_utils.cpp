#include "editor_utils.h"

#include "engine/foundation/base.h"
#include "engine/core/profiler.h"
#include "engine/project/project.h"
#include "editor/project/project_serializer.h"
#include "engine/serialization/scene_serializer.h"

#include <algorithm>
#include <format>
#include <vector>

#if CH_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <shellapi.h>
#endif

namespace Chained
{

// --- Internal Helper Declarations ---
static std::filesystem::path FindRuntimeExecutable(const std::string& projectName, const std::string& configStr);
static std::string ResolveLaunchVariables(std::string str, std::shared_ptr<Project> project);

// --- Public Function Implementations ---

void LaunchStandalone(std::shared_ptr<Project> project, std::shared_ptr<Scene> editorScene)
{
    CH_PROFILE_FUNCTION();

    if (!project)
    {
        CH_CORE_ERROR("LaunchStandalone: No active project to launch!");
        return;
    }

    auto& config = project->GetConfig();
    std::string sceneArgument;

    if (editorScene)
    {
        std::filesystem::path scenePath = editorScene->GetSettings().ScenePath;
        if (scenePath.empty())
        {
            scenePath = config.ActiveScenePath;
        }

        if (!scenePath.empty())
        {
            // Sync scene to disk before launching standalone
            if (!editorScene->GetSettings().ScenePath.empty())
            {
                SceneSerializer serializer(editorScene.get());
                if (!serializer.Serialize(editorScene->GetSettings().ScenePath))
                {
                    CH_CORE_ERROR("LaunchStandalone: Failed to save current editor scene before launching.");
                    return;
                }
            }

            if (scenePath.is_relative())
            {
                scenePath = Project::GetAssetPath(scenePath);
            }

            scenePath = std::filesystem::absolute(scenePath);
            project->GetConfig().ActiveScenePath = Project::GetRelativePath(scenePath);
            sceneArgument = std::format(" --scene \"{}\"", scenePath.string());
        }
    }



    std::string runtimePath;
    std::string arguments;

    std::string configStr = (config.BuildConfig == Configuration::Release) ? "Release" : "Debug";
    runtimePath = FindRuntimeExecutable(config.Name, configStr).string();

    std::filesystem::path projectFile =
        project->GetProjectDirectoryForProject() / (project->GetConfig().Name + ".chproject");
    arguments = std::format("\"{}\"", std::filesystem::absolute(projectFile).string());

    if (!sceneArgument.empty())
    {
        arguments += sceneArgument;
    }

    if (runtimePath.empty() || !std::filesystem::exists(runtimePath))
    {
        CH_CORE_WARN("LaunchStandalone: Profile binary not found at '{}'. Searching heuristic...", runtimePath);
        runtimePath = FindRuntimeExecutable(config.Name, configStr).string();

        if (runtimePath.empty())
        {
            CH_CORE_ERROR("LaunchStandalone: Runtime executable not found!");
            return;
        }
    }

#if CH_PLATFORM_WINDOWS
    // Normalize slashes for Windows (start command and ShellExecute prefer \)
    std::string normalizedRuntime = runtimePath;
    std::replace(normalizedRuntime.begin(), normalizedRuntime.end(), '/', '\\');

    std::string normalizedArgs = arguments;
    std::replace(normalizedArgs.begin(), normalizedArgs.end(), '/', '\\');

    CH_CORE_INFO("LaunchStandalone: Executing via ShellExecute: {} {}", normalizedRuntime, normalizedArgs);

    // Use ShellExecute instead of system to be truly non-blocking and avoid cmd window issues
    HINSTANCE result =
        ShellExecuteW(NULL, L"open", std::wstring(normalizedRuntime.begin(), normalizedRuntime.end()).c_str(),
                      std::wstring(normalizedArgs.begin(), normalizedArgs.end()).c_str(), NULL, SW_SHOW);
    if ((uintptr_t)result <= 32)
    {
        CH_CORE_ERROR("LaunchStandalone: ShellExecute failed with error code: {}", (uintptr_t)result);
    }
#else
    std::string command = std::format("\"{}\" {} &", runtimePath, arguments);
    CH_CORE_INFO("LaunchStandalone: Executing: {}", command);
    system(command.c_str());
#endif
}

// --- Internal Helper Implementations ---

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

#if CH_PLATFORM_WINDOWS
    const std::string targetName = "ChainedRuntime.exe";
#else
    const std::string targetName = "ChainedRuntime";
#endif

    // 1. Check current working directory
    std::filesystem::path currentBin = std::filesystem::current_path() / targetName;
    if (std::filesystem::exists(currentBin))
    {
        return currentBin;
    }

    // 2. Fast common output locations
    std::vector<std::string> searchSubdirs = {"build/bin", "bin", "out/bin", "cmake-build-debug/bin",
                                              "cmake-build-release/bin"};

    // Auto-discover build folders in project root
    if (std::filesystem::exists(root / "build"))
    {
        for (const auto& entry : std::filesystem::directory_iterator(root / "build"))
        {
            if (entry.is_directory())
            {
                if (std::filesystem::exists(entry.path() / "bin" / targetName))
                {
                    searchSubdirs.push_back("build/" + entry.path().filename().string() + "/bin");
                }
            }
        }
    }

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
                CH_CORE_INFO("FindRuntimeExecutable: Deep search found at: {}", entry.path().string());
                return entry.path();
            }
        }
    } catch (const std::exception& e)
    {
        CH_CORE_WARN("FindRuntimeExecutable: Deep search error: {}", e.what());
    }

    return {};
}

static std::string ResolveLaunchVariables(std::string str, std::shared_ptr<Project> project)
{
    CH_PROFILE_FUNCTION();

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

    std::filesystem::path projectFile =
        project->GetProjectDirectoryForProject() / (project->GetConfig().Name + ".chproject");
    std::string projectPathStr = std::filesystem::absolute(projectFile).string();

    auto replaceAll = [&](const std::string& from, const std::string& to) {
        size_t pos = 0;
        while ((pos = str.find(from)) != std::string::npos)
        {
            str.replace(pos, from.length(), to);
        }
    };

    replaceAll("${ROOT}", std::filesystem::absolute(root).string());
    replaceAll("${PROJECT_FILE}", projectPathStr);

    if (str.find("${BUILD}") != std::string::npos)
    {
        std::string configStr = (project->GetConfig().BuildConfig == Configuration::Release) ? "Release" : "Debug";
        std::filesystem::path exePath = FindRuntimeExecutable(project->GetConfig().Name, configStr);
        std::filesystem::path buildPath = exePath.parent_path();

        if (buildPath.empty())
        {
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

        replaceAll("${BUILD}", std::filesystem::absolute(buildPath).string());
    }

    return str;
}

} // namespace Chained
