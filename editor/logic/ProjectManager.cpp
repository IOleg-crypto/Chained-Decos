#include "ProjectManager.h"
#include "ProjectData.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

ProjectManager::ProjectManager()
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

    // TODO: Acquire current project data from application state
    ProjectData data;
    data.name = fs::path(m_projectPath).stem().string();

    std::ofstream file(m_projectPath);
    if (file.is_open())
    {
        file << data.ToJson().dump(4);
        file.close();
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

        // TODO: Notify systems about project load
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
