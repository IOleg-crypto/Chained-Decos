#include "project_manager.h"
#include "core/log.h"
#include "engine/scene/core/scene_manager.h"
#include <filesystem>

namespace CHEngine
{
ProjectManager *ProjectManager::s_Instance = nullptr;

ProjectManager &ProjectManager::Get()
{
    return *s_Instance;
}

void ProjectManager::Init()
{
    if (!s_Instance)
        s_Instance = new ProjectManager();
}

void ProjectManager::Shutdown()
{
    delete s_Instance;
    s_Instance = nullptr;
}

ProjectManager::ProjectManager()
{
}

std::shared_ptr<Project> ProjectManager::NewProject(const std::string &name,
                                                    const std::string &location)
{
    m_ActiveProject = Project::Create(location, name);
    if (m_ActiveProject)
    {
        SceneManager::Get().NewScene();
    }
    return m_ActiveProject;
}

std::shared_ptr<Project> ProjectManager::OpenProject(const std::string &path)
{
    m_ActiveProject = Project::Load(path);
    if (m_ActiveProject)
    {
        std::string startScene = m_ActiveProject->GetConfig().startScene;
        if (!startScene.empty())
        {
            std::filesystem::path scenePath = m_ActiveProject->GetAbsolutePath(startScene);
            if (std::filesystem::exists(scenePath))
            {
                SceneManager::Get().OpenScene(scenePath.string());
            }
        }
    }
    return m_ActiveProject;
}

void ProjectManager::SaveProject()
{
    if (m_ActiveProject)
        m_ActiveProject->Save();
}

void ProjectManager::CloseProject()
{
    if (m_ActiveProject)
    {
        SaveProject();
        m_ActiveProject.reset();
        SceneManager::Get().NewScene();
    }
}
} // namespace CHEngine
