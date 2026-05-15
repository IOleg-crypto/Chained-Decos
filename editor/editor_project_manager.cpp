#include "editor_project_manager.h"
#include "editor_layer.h"
#include "engine/scene/project.h"
#include "engine/scene/project_serializer.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/ui_renderer.h"
#include "engine/core/application.h"
#include "engine/scene/scene_events.h"
#include "engine/core/platform.h"
#include "scripting/scriptengine.h"
#include "engine/core/service_locator.h"
#include <algorithm>

namespace CHEngine
{

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
    Project::New();
    auto project = Project::GetActive();
    project->GetConfig().Name = name;
    project->GetConfig().ProjectDirectory = path;

    ProjectSerializer serializer(project);
    serializer.Serialize((std::filesystem::path(path) / (name + ".chproject")).string());

    // Load engine shaders and resources for the dynamic newly created project
    ServiceLocator::Get<Renderer>().LoadEngineResources();
    ServiceLocator::Get<UIRenderer>().LoadProjectFonts();
}

void EditorProjectManager::OpenProject()
{
    std::vector<FileDialogFilter> filters = {{"Chained Project", "chproject"}};
    auto result = CHEngine::Platform::OpenFile(filters);
    if (result)
    {
        OpenProject(*result);
    }
}

void EditorProjectManager::OpenProject(const std::filesystem::path& path)
{
    if (Project::Load(path))
    {
        m_LastProjectPath = path.string();

        // Load engine shaders and resources
        ServiceLocator::Get<Renderer>().LoadEngineResources();
        ServiceLocator::Get<UIRenderer>().LoadProjectFonts();

        ProjectOpenedEvent e(path.string());
        Application::Get().OnEvent(e);
    }
}

void EditorProjectManager::SaveProject()
{
    auto project = Project::GetActive();
    if (!project) return;
    
    ProjectSerializer serializer(project);
    serializer.Serialize((project->GetConfig().ProjectDirectory / (project->GetConfig().Name + ".chproject")).string());
}

bool EditorProjectManager::OnProjectOpened(ProjectOpenedEvent& e)
{
    auto project = Project::GetActive();
    if (project)
    {
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
                ServiceLocator::Get<ScriptEngine>().LoadAppAssembly(dllPath.string());
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

} // namespace CHEngine
