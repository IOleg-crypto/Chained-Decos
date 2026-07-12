#include "engine/platform/dialogs/file_dialogs.h"
#include "engine/core/service_locator.h"
#include "project_manager.h"
#include "layer.h"
#include "engine/project/project.h"
#include "project/project_serializer.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/ui/ui_renderer.h"
#include "engine/scene/scene_events.h"
#include "scripting/scriptengine.h"
#include "engine/assets/asset_manager.h"
#include <algorithm>
#include <string>
#include <format>
#include "engine/scene/scene_serializer.h"
#include "engine/core/profiler.h"

#if CH_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <shellapi.h>
#endif

namespace Chained
{

static std::filesystem::path FindProjectRoot()
{
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
    return root;
}

static const std::vector<std::string>& GetSearchSubdirs()
{
    static const std::vector<std::string> subdirs = {
        "build/bin", "bin", "out/bin", "cmake-build-debug/bin", "cmake-build-release/bin"};
    return subdirs;
}

static std::filesystem::path FindRuntimeExecutable(const std::string& projectName, const std::string& configStr)
{
    CH_PROFILE_FUNCTION();

    std::filesystem::path root = FindProjectRoot();

    if (!std::filesystem::exists(root))
    {
        CH_CORE_ERROR("FindRuntimeExecutable: Root path not found: {}", root.string());
        return {};
    }

#if CH_PLATFORM_WINDOWS
    const std::string perGameName = projectName + ".exe";
    const std::string fallbackName = "ChainedRuntime.exe";
#else
    const std::string perGameName = projectName;
    const std::string fallbackName = "ChainedRuntime";
#endif

    auto searchFor = [&](const std::string& targetName) -> std::filesystem::path {
        std::filesystem::path currentBin = std::filesystem::current_path() / targetName;
        if (std::filesystem::exists(currentBin))
            return currentBin;

        auto searchSubdirs = GetSearchSubdirs();

        if (std::filesystem::exists(root / "build"))
        {
            for (const auto& entry : std::filesystem::directory_iterator(root / "build"))
            {
                if (entry.is_directory())
                {
                    if (std::filesystem::exists(entry.path() / "bin" / targetName))
                        searchSubdirs.push_back("build/" + entry.path().filename().string() + "/bin");
                }
            }
        }

        for (const auto& sub : searchSubdirs)
        {
            std::filesystem::path p = root / sub / targetName;
            if (std::filesystem::exists(p))
            {
                CH_CORE_INFO("FindRuntimeExecutable: Found '{}' at: {}", targetName, p.string());
                return p;
            }
        }
        return {};
    };

    auto result = searchFor(perGameName);
    if (!result.empty()) return result;

    result = searchFor(fallbackName);
    if (!result.empty()) return result;

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

            if (entry.is_regular_file() && (filename == perGameName || filename == fallbackName))
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
    if (!project) return str;

    std::filesystem::path root = FindProjectRoot();

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
            for (const auto& sub : GetSearchSubdirs())
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

EditorProjectManager::EditorProjectManager()
{
}

void EditorProjectManager::NewProject()
{
    // Simple default: close active project to show Project Browser
    Project::SetActive(nullptr);
}

void EditorProjectManager::NewProject(const std::string& name, const std::string& path)
{
    auto project = std::make_shared<Project>();
    project->GetConfig().Name = name;
    project->GetConfig().ProjectDirectory = path;

    m_EditorSettings = EditorSettings(); // Reset to defaults

    EditorProjectSerializer::Serialize(project, m_EditorSettings, (std::filesystem::path(path) / (name + ".chproject")));

    Project::SetActive(project);
    
    ProjectOpenedEvent e((std::filesystem::path(path) / (name + ".chproject")).string());
    Application::Get().OnEvent(e);
}

void EditorProjectManager::OpenProject()
{
    std::vector<FileDialogFilter> filters = {{"Chained Project", "chproject"}};
    auto result = Chained::FileDialogs::OpenFile(filters);
    if (result)
    {
        OpenProject(*result);
    }
}

void EditorProjectManager::OpenProject(const std::filesystem::path& path)
{
    auto project = std::make_shared<Project>();
    if (EditorProjectSerializer::Deserialize(project, m_EditorSettings, path))
    {
        m_LastProjectPath = path.string();
        Project::SetActive(project);


         ProjectOpenedEvent e(path.string());
        Application::Get().OnEvent(e);
    }
}

void EditorProjectManager::SaveProject()
{
    auto project = Project::GetActive();
    if (!project) return;
    
    EditorProjectSerializer::Serialize(project, m_EditorSettings, (project->GetConfig().ProjectDirectory / (project->GetConfig().Name + ".chproject")));
}


bool EditorProjectManager::OnProjectOpened(ProjectOpenedEvent& e)
{
    auto project = Project::GetActive();
    if (project)
    {
        std::filesystem::path resolvedPath = e.GetPath();
        std::filesystem::path projDir = resolvedPath.extension() == ".chproject" ? resolvedPath.parent_path() : resolvedPath;

        ServiceLocator::Get<AssetManager>()->SetProjectDirectory(project->GetProjectDirectoryForProject());
        ServiceLocator::Get<AssetManager>()->SetAssetDirectory(project->GetAssetDirectoryForProject());
 
         // Load engine shaders and resources
        ServiceLocator::Get<Renderer>()->LoadEngineResources();
        ServiceLocator::Get<UIRenderer>()->LoadProjectFonts();

        m_LastProjectPath = e.GetPath();

        // Track in recent projects list (move to front, cap at 10)
        auto& config = EditorLayer::Get().GetConfig();
        auto& recents = config.RecentProjects;
        recents.erase(std::remove(recents.begin(), recents.end(), m_LastProjectPath), recents.end());
        recents.insert(recents.begin(), m_LastProjectPath);
        if (recents.size() > 10)
            recents.resize(10);

        EditorLayer::Get().SaveConfig();

        // Auto-load script assembly if configured
        auto& scripting = project->GetConfig().Scripting;
        if (scripting.AutoLoad && !scripting.ModuleName.empty())
        {
            std::string dllName = scripting.ModuleName;
            if (dllName.find(".dll") == std::string::npos)
                dllName += ".dll";

            std::filesystem::path dllPath = scripting.ModuleDirectory / dllName;
            if (dllPath.is_relative())
                dllPath = project->GetConfig().ProjectDirectory / dllPath;

            if (std::filesystem::exists(dllPath))
            {
                 ServiceLocator::Get<ScriptEngine>()->SetEnabled(true);
                 ServiceLocator::Get<ScriptEngine>()->Initialize();
                 ServiceLocator::Get<ScriptEngine>()->LoadAppAssembly(dllPath.string());
                 CH_CORE_INFO("EditorProjectManager: Auto-loaded script assembly '{}'.", dllPath.string());
            }
            else
            {
                CH_CORE_WARN("EditorProjectManager: Script assembly not found at '{}'. Build the C# project first.",
                             dllPath.string());
            }
        }

        // Auto-load scene if available
        std::filesystem::path sceneToLoad;

        // 1. Try loading ActiveScene
        if (!project->GetConfig().ActiveScenePath.empty())
        {
            sceneToLoad = project->GetConfig().ProjectDirectory / project->GetConfig().ActiveScenePath;
        }

        // 2. Fallback to StartScene
        if (sceneToLoad.empty() || !std::filesystem::exists(sceneToLoad))
        {
            if (!project->GetConfig().StartScene.empty())
            {
                sceneToLoad = project->GetConfig().ProjectDirectory / project->GetConfig().AssetDirectory /
                               project->GetConfig().StartScene;
            }
        }

        // 3. Load the scene if found
        if (!sceneToLoad.empty() && std::filesystem::exists(sceneToLoad))
        {
            CH_CORE_INFO("EditorProjectManager: Auto-loading scene: {}", sceneToLoad.string());
            EditorLayer::Get().GetSceneManager().OpenScene(sceneToLoad);
        }
        return true;
    }
    return false;
}

const std::string & EditorProjectManager::GetLastProjectPath() const {
    return m_LastProjectPath;
}

void EditorProjectManager::SetLastProjectPath(const std::string &path) {
    m_LastProjectPath = path;
}
void EditorProjectManager::LaunchStandalone(std::shared_ptr<Scene> editorScene)
{
    CH_PROFILE_FUNCTION();
    auto project = Project::GetActive();
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

    std::string configStr = (config.BuildConfig == Configuration::Release) ? "Release" : "Debug";
    std::string runtimePath = FindRuntimeExecutable(config.Name, configStr).string();

    std::filesystem::path projectFile = project->GetProjectDirectoryForProject() / (project->GetConfig().Name + ".chproject");
    std::string arguments = std::format("\"{}\"", std::filesystem::absolute(projectFile).string());

    if (!sceneArgument.empty())
    {
        arguments += sceneArgument;
    }

    if (runtimePath.empty() || !std::filesystem::exists(runtimePath))
    {
        CH_CORE_WARN("LaunchStandalone: Runtime binary not found at '{}'. Searching heuristic...", runtimePath);
        runtimePath = FindRuntimeExecutable(config.Name, configStr).string();

        if (runtimePath.empty() || !std::filesystem::exists(runtimePath))
        {
            CH_CORE_ERROR("LaunchStandalone: Runtime executable not found! Searched for '{}.exe' and 'ChainedRuntime.exe'.", config.Name);
            return;
        }
    }

    if (!std::filesystem::exists(projectFile))
    {
        CH_CORE_ERROR("LaunchStandalone: Project file not found: {}", std::filesystem::absolute(projectFile).string());
        return;
    }

#if CH_PLATFORM_WINDOWS
    std::string normalizedRuntime = runtimePath;
    std::replace(normalizedRuntime.begin(), normalizedRuntime.end(), '/', '\\');

    std::string normalizedArgs = arguments;
    std::replace(normalizedArgs.begin(), normalizedArgs.end(), '/', '\\');

    CH_CORE_INFO("LaunchStandalone: Executing via ShellExecute: {} {}", normalizedRuntime, normalizedArgs);

    // Provide the executable's directory as the working directory so it doesn't inherit the editor's CWD
    std::string exeDir = std::filesystem::path(normalizedRuntime).parent_path().string();

    std::wstring wExeDir(exeDir.begin(), exeDir.end());
    HINSTANCE result =
        ShellExecuteW(NULL, L"open", std::wstring(normalizedRuntime.begin(), normalizedRuntime.end()).c_str(),
                      std::wstring(normalizedArgs.begin(), normalizedArgs.end()).c_str(), wExeDir.c_str(), SW_SHOW);
    if ((uintptr_t)result <= 32)
    {
        DWORD err = GetLastError();
        CH_CORE_ERROR("LaunchStandalone: ShellExecute failed with code {} (Win32 error: {})", (uintptr_t)result, err);
    }
#else
    std::string command = std::format("\"{}\" {} &", runtimePath, arguments);
    CH_CORE_INFO("LaunchStandalone: Executing: {}", command);
    system(command.c_str());
#endif
}

} // namespace Chained
