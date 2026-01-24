#include "editor_utils.h"
#include "editor/editor_settings.h"
#include "engine/core/application.h"
#include "engine/core/log.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene_serializer.h"
#include "nfd.hpp"
#include <algorithm>

namespace CHEngine::EditorUtils
{
namespace ProjectUtils
{
void NewProject()
{
    nfdchar_t *savePath = NULL;
    nfdu8filteritem_t filterItem[1] = {{"Chained Project", "chproj"}};
    nfdresult_t result = NFD_SaveDialog(&savePath, filterItem, 1, NULL, "Untitled.chproj");
    if (result == NFD_OKAY)
    {
        std::string path = savePath;
        std::string name = std::filesystem::path(path).stem().string();
        NewProject(name, path);
        NFD_FreePath(savePath);
    }
}

void NewProject(const std::string &name, const std::string &path)
{
    CH_CORE_INFO("ProjectUtils: NewProject request - Name: '{0}', Path: '{1}'", name, path);

    std::filesystem::path rootPath = std::filesystem::path(path).lexically_normal();
    if (rootPath.has_filename() && rootPath.filename() == ".")
        rootPath = rootPath.parent_path();

    std::filesystem::path projectDir;
    std::string folderName = rootPath.filename().string();

    // Windows case-insensitive comparison
    std::string folderLower = folderName;
    std::transform(folderLower.begin(), folderLower.end(), folderLower.begin(), ::tolower);
    std::string nameLower = name;
    std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

    if (!folderLower.empty() && folderLower == nameLower)
        projectDir = rootPath;
    else
        projectDir = rootPath / name;

    try
    {
        if (!std::filesystem::exists(projectDir))
            std::filesystem::create_directories(projectDir);
    }
    catch (const std::exception &e)
    {
        CH_CORE_ERROR("ProjectUtils: Failed to create project directory: {0}", e.what());
        return;
    }

    std::filesystem::path projectFilePath = projectDir / (name + ".chproj");
    Ref<Project> newProject = CreateRef<Project>();
    newProject->SetName(name);
    newProject->SetProjectDirectory(projectDir);

    ProjectSerializer serializer(newProject);
    if (serializer.Serialize(projectFilePath))
    {
        Project::SetActive(newProject);
        EditorSettings::SetLastProjectPath(projectFilePath.string());
        EditorSettings::Save();

        std::filesystem::create_directories(projectDir / "assets/scenes");

        // Dispatch event so EditorLayer/Panels can update
        ProjectOpenedEvent e(projectFilePath.string());
        Application::OnEvent(e);

        // Automatically create a new scene
        SceneUtils::NewScene();
    }
}

void OpenProject()
{
    nfdchar_t *outPath = NULL;
    nfdu8filteritem_t filterItem[1] = {{"Chained Project", "chproj"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
    if (result == NFD_OKAY)
    {
        OpenProject(outPath);
        NFD_FreePath(outPath);
    }
}

void OpenProject(const std::filesystem::path &path)
{
    CH_CORE_INFO("ProjectUtils: Opening project: {0}", path.string());
    Ref<Project> project = CreateRef<Project>();
    ProjectSerializer serializer(project);
    if (serializer.Deserialize(path))
    {
        Project::SetActive(project);
        EditorSettings::SetLastProjectPath(path.string());
        EditorSettings::Save();

        ProjectOpenedEvent e(path.string());
        Application::OnEvent(e);

        auto &config = project->GetConfig();
        if (!config.ActiveScenePath.empty())
        {
            std::filesystem::path scenePath =
                project->GetProjectDirectory() / config.ActiveScenePath;
            SceneUtils::OpenScene(scenePath);
        }
    }
}

void SaveProject()
{
    auto project = Project::GetActive();
    if (project)
    {
        // Auto-save the current scene as well
        SceneUtils::SaveScene();

        std::filesystem::path path =
            project->GetProjectDirectory() / (project->GetConfig().Name + ".chproj");
        ProjectSerializer serializer(project);
        serializer.Serialize(path.string());
    }
}
} // namespace ProjectUtils

namespace SceneUtils
{
void NewScene()
{
    Ref<Scene> newScene = CreateRef<Scene>();
    Application::Get().SetActiveScene(newScene);

    SceneOpenedEvent e("");
    Application::OnEvent(e);
}

void OpenScene()
{
    nfdchar_t *outPath = NULL;
    nfdu8filteritem_t filterItem[1] = {{"Chained Scene", "chscene"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
    if (result == NFD_OKAY)
    {
        OpenScene(outPath);
        NFD_FreePath(outPath);
    }
}

void OpenScene(const std::filesystem::path &path)
{
    CH_CORE_INFO("SceneUtils: Opening scene: {0}", path.string());
    Ref<Scene> newScene = CreateRef<Scene>();
    SceneSerializer serializer(newScene.get());
    if (serializer.Deserialize(path.string()))
    {
        Application::Get().SetActiveScene(newScene);

        auto project = Project::GetActive();
        if (project)
        {
            project->SetActiveScenePath(
                std::filesystem::relative(path, project->GetProjectDirectory()));
            ProjectUtils::SaveProject();
        }

        SceneOpenedEvent e(path.string());
        Application::OnEvent(e);
    }
}

void SaveScene()
{
    // Logic for SaveScene would need access to the current scene path.
    // We can get it from Project or a static variable.
    auto project = Project::GetActive();
    if (project && !project->GetConfig().ActiveScenePath.empty())
    {
        std::filesystem::path path =
            project->GetProjectDirectory() / project->GetConfig().ActiveScenePath;
        SceneSerializer serializer(Application::Get().GetActiveScene().get());
        serializer.Serialize(path.string());
        CH_CORE_INFO("Scene Saved: {0}", path.string());
    }
    else
    {
        SaveSceneAs();
    }
}

void SaveSceneAs()
{
    nfdchar_t *savePath = NULL;
    nfdu8filteritem_t filterItem[1] = {{"Chained Scene", "chscene"}};
    nfdresult_t result = NFD_SaveDialog(&savePath, filterItem, 1, NULL, "Untitled.chscene");
    if (result == NFD_OKAY)
    {
        std::string path = savePath;
        if (std::filesystem::path(path).extension() != ".chscene")
            path += ".chscene";

        SceneSerializer serializer(Application::Get().GetActiveScene().get());
        if (serializer.Serialize(path))
        {
            auto project = Project::GetActive();
            if (project)
            {
                project->SetActiveScenePath(
                    std::filesystem::relative(path, project->GetProjectDirectory()));
                ProjectUtils::SaveProject();
            }

            SceneOpenedEvent e(path);
            Application::OnEvent(e);
        }
        NFD_FreePath(savePath);
    }
}
} // namespace SceneUtils
} // namespace CHEngine::EditorUtils
