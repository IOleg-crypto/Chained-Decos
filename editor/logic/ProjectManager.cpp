#include "ProjectManager.h"
#include "core/Base.h"
#include "scene/resources/map/SceneLoader.h"
#include "scene/resources/map/SceneSerializer.h"
#include <filesystem>
#include <nfd.h>

namespace CHEngine
{

ProjectManager::ProjectManager()
{
    m_ActiveScene = std::make_shared<GameScene>();
}

// =========================================================================
// Project Operations
// =========================================================================

std::shared_ptr<Project> ProjectManager::NewProject(const std::string &name,
                                                    const std::string &location)
{
    m_ActiveProject = Project::Create(location, name);
    if (m_ActiveProject)
    {
        NewScene(); // Create initial empty scene in the project context
    }
    return m_ActiveProject;
}

std::shared_ptr<Project> ProjectManager::OpenProject(const std::string &path)
{
    m_ActiveProject = Project::Load(path);
    if (m_ActiveProject)
    {
        // Try to load the start scene if it exists
        std::string startScene = m_ActiveProject->GetConfig().startScene;
        if (!startScene.empty())
        {
            std::filesystem::path scenePath = m_ActiveProject->GetAbsolutePath(startScene);
            if (std::filesystem::exists(scenePath))
            {
                OpenScene(scenePath.string());
            }
        }
    }
    return m_ActiveProject;
}

void ProjectManager::SaveProject()
{
    if (m_ActiveProject)
    {
        m_ActiveProject->Save();
    }
}

void ProjectManager::CloseProject()
{
    if (m_ActiveProject)
    {
        SaveProject();
        m_ActiveProject.reset();
        m_ActiveScene = std::make_shared<GameScene>();
        m_ScenePath = "";
        if (m_SceneChangedCallback)
            m_SceneChangedCallback(m_ActiveScene);
    }
}

// =========================================================================
// Scene Operations
// =========================================================================

void ProjectManager::NewScene()
{
    m_ActiveScene = std::make_shared<GameScene>();
    m_ScenePath = "";

    if (m_SceneChangedCallback)
        m_SceneChangedCallback(m_ActiveScene);
}

bool ProjectManager::OpenScene()
{
    nfdchar_t *outPath = nullptr;
    nfdfilteritem_t filterItem[1] = {{"Chained Decos Scene", "chscene"}};

    // Use project scene directory as default if available
    const char *defaultPath = nullptr;
    std::string defaultPathStr;
    if (m_ActiveProject)
    {
        defaultPathStr = m_ActiveProject->GetSceneDirectory().string();
        defaultPath = defaultPathStr.c_str();
    }

    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, defaultPath);

    if (result == NFD_OKAY)
    {
        bool success = OpenScene(outPath);
        NFD_FreePath(outPath);
        return success;
    }
    return false;
}

bool ProjectManager::OpenScene(const std::string &path)
{
    auto newScene = std::make_shared<GameScene>();
    SceneSerializer serializer(newScene);

    if (!serializer.DeserializeBinary(path))
        return false;

    m_ActiveScene = newScene;
    m_ScenePath = path;

    SceneLoader().LoadSkyboxForScene(*m_ActiveScene);

    if (m_SceneChangedCallback)
        m_SceneChangedCallback(m_ActiveScene);

    return true;
}

void ProjectManager::SaveScene()
{
    if (m_ScenePath.empty())
    {
        SaveSceneAs();
    }
    else
    {
        SceneSerializer serializer(m_ActiveScene);
        serializer.SerializeBinary(m_ScenePath);
    }
}

void ProjectManager::SaveSceneAs()
{
    nfdchar_t *outPath = nullptr;
    nfdfilteritem_t filterItem[1] = {{"Chained Decos Scene", "chscene"}};

    // Use project scene directory as default if available
    const char *defaultPath = nullptr;
    std::string defaultPathStr;
    if (m_ActiveProject)
    {
        defaultPathStr = (m_ActiveProject->GetSceneDirectory() / "NewScene.chscene").string();
        defaultPath = defaultPathStr.c_str();
    }

    nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, defaultPath, "NewScene.chscene");

    if (result == NFD_OKAY)
    {
        m_ScenePath = outPath;
        if (m_ScenePath.find(".chscene") == std::string::npos)
            m_ScenePath += ".chscene";

        SceneSerializer serializer(m_ActiveScene);
        serializer.SerializeBinary(m_ScenePath);
        NFD_FreePath(outPath);
    }
}

// =========================================================================
// Getters & Setters
// =========================================================================

std::shared_ptr<GameScene> ProjectManager::GetActiveScene() const
{
    return m_ActiveScene;
}

void ProjectManager::SetActiveScene(std::shared_ptr<GameScene> scene)
{
    m_ActiveScene = scene;
}

const std::string &ProjectManager::GetScenePath() const
{
    return m_ScenePath;
}

void ProjectManager::SetSceneChangedCallback(
    std::function<void(std::shared_ptr<GameScene>)> callback)
{
    m_SceneChangedCallback = callback;
}

} // namespace CHEngine
