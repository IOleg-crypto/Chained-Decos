#include "engine/platform/utils/file_dialogs.h"
#include "engine/core/service_locator.h"
#include "editor_project_manager.h"
#include "editor_layer.h"
#include "engine/project/project.h"
#include "project/project_serializer.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/ui/ui_renderer.h"
#include "engine/scene/scene_events.h"
#include "scripting/scriptengine.h"
#include "engine/assets/asset_manager.h"
#include <algorithm>
#include <string>

namespace Chained
{

EditorProjectManager::EditorProjectManager(EditorLayer& owner)
    : m_EditorLayer(owner)
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

        ServiceLocator::Get<AssetManager>()->SetProjectDirectory(projDir);
        ServiceLocator::Get<AssetManager>()->SetAssetDirectory(projDir / "assets");
 
         // Load engine shaders and resources
        ServiceLocator::Get<Renderer>()->LoadEngineResources();
        ServiceLocator::Get<Renderer>()->GetUIRenderer()->LoadProjectFonts();

        m_LastProjectPath = e.GetPath();

        // Track in recent projects list (move to front, cap at 10)
        auto& config = m_EditorLayer.GetConfig();
        auto& recents = config.RecentProjects;
        recents.erase(std::remove(recents.begin(), recents.end(), m_LastProjectPath), recents.end());
        recents.insert(recents.begin(), m_LastProjectPath);
        if (recents.size() > 10)
            recents.resize(10);

        m_EditorLayer.SaveConfig();

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
                 ScriptEngine::Get().LoadAppAssembly(dllPath.string());
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
            m_EditorLayer.GetSceneManager().OpenScene(sceneToLoad);
        }
        return true;
    }
    return false;
}

} // namespace Chained
