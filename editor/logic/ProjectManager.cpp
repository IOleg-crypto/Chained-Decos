#include "ProjectManager.h"
#include "ProjectData.h"
#include "core/Log.h"
#include "editor/IEditor.h"
#include "editor/logic/ISceneManager.h"
#include "editor/panels/AssetBrowserPanel.h"
#include "editor/panels/EditorPanelManager.h"
#include <filesystem>
#include <fstream>
#include <iostream>


namespace fs = std::filesystem;

ProjectManager::ProjectManager(IEditor *editor) : m_editor(editor)
{
}

ProjectManager::~ProjectManager()
{
}

void ProjectManager::SetProjectPath(const std::string &path)
{
    m_projectPath = path;
}

bool ProjectManager::CreateNewProject(const std::string &path)
{
    try
    {
        fs::path projectPath(path);
        if (!fs::exists(projectPath))
        {
            fs::create_directories(projectPath);
        }

        // Create directory structure
        fs::create_directories(projectPath / "Assets");
        fs::create_directories(projectPath / "Scenes");
        fs::create_directories(projectPath / "Scripts");

        // Create project file
        ProjectData initialData;
        initialData.name = projectPath.filename().string();

        std::string projectFilePath = (projectPath / (initialData.name + ".cdproj")).string();

        std::ofstream file(projectFilePath);
        if (file.is_open())
        {
            file << initialData.ToJson().dump(4);
            file.close();

            m_projectPath = projectFilePath;
            AddRecentProject(projectFilePath);

            // Create a default scene
            std::string defaultSceneName = "MainScene.json";
            fs::path scenePath = projectPath / "Scenes" / defaultSceneName;
            m_editor->GetSceneManager().ClearScene();
            m_editor->GetSceneManager().GetGameScene().GetMapMetaDataMutable().name = "MainScene";
            m_editor->GetSceneManager().SaveScene(scenePath.string());

            // Update project with last scene
            initialData.lastScene = defaultSceneName;
            std::ofstream updateFile(projectFilePath);
            if (updateFile.is_open())
            {
                updateFile << initialData.ToJson().dump(4);
                updateFile.close();
            }

            // Update Asset Browser root to project directory
            auto assetBrowser =
                m_editor->GetPanelManager().GetPanel<AssetBrowserPanel>("AssetBrowser");
            if (assetBrowser)
            {
                assetBrowser->SetRootPath(path);
            }

            return true;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error creating project: " << e.what() << std::endl;
    }

    return false;
}

void ProjectManager::SaveProject()
{
    if (m_projectPath.empty())
        return;

    // Acquire current project data from application state
    ProjectData data;
    data.name = fs::path(m_projectPath).stem().string();

    if (m_editor)
    {
        fs::path scenePath(m_editor->GetSceneManager().GetCurrentMapPath());
        data.lastScene = scenePath.filename().string();
        data.gridSize = m_editor->GetState().GetGridSize();
        data.drawWireframe = m_editor->GetState().IsWireframeEnabled();
        data.drawCollisions = m_editor->GetState().IsCollisionDebugEnabled();
    }

    std::ofstream file(m_projectPath);
    if (file.is_open())
    {
        file << data.ToJson().dump(4);
        file.close();
        CD_INFO("[ProjectManager] Project saved: %s", m_projectPath.c_str());
    }
}

void ProjectManager::LoadProject(const std::string &path)
{
    if (!fs::exists(path))
        return;

    std::ifstream file(path);
    if (file.is_open())
    {
        json j;
        file >> j;
        ProjectData data = ProjectData::FromJson(j);

        m_projectPath = path;
        AddRecentProject(path);

        // Automatically load the last scene if present
        if (m_editor && !data.lastScene.empty())
        {
            // The scene path in ProjectData is relative to project root
            fs::path projectRoot = fs::path(path).parent_path();
            fs::path sceneFull = projectRoot / "Scenes" / data.lastScene;
            m_editor->GetSceneManager().LoadScene(sceneFull.string());
        }
        else if (m_editor)
        {
            // If no last scene, clear or create default
            m_editor->GetSceneManager().ClearScene();
        }

        // Update Asset Browser root to project directory
        auto assetBrowser = m_editor->GetPanelManager().GetPanel<AssetBrowserPanel>("AssetBrowser");
        if (assetBrowser)
        {
            assetBrowser->SetRootPath(fs::path(path).parent_path().string());
        }
    }
}

const std::vector<std::string> &ProjectManager::GetRecentProjects() const
{
    static std::vector<std::string> recent;
    // TODO: Load from editor settings
    return recent;
}

void ProjectManager::AddRecentProject(const std::string &path)
{
    // TODO: Implementation for persistent recent projects list
}
const std::string &ProjectManager::GetProjectPath() const
{
    return m_projectPath;
}
