#ifndef CH_PROJECT_H
#define CH_PROJECT_H

#include "engine/base.h"
#include <filesystem>
#include <string>

namespace CH
{
struct ProjectConfig
{
    std::string Name = "Untitled";
    std::string StartScene;
    std::filesystem::path AssetDirectory = "assets";
    std::filesystem::path ProjectDirectory;
    std::filesystem::path ActiveScenePath;
};

class Project
{
public:
    Project() = default;
    ~Project() = default;

    const ProjectConfig &GetConfig() const
    {
        return m_Config;
    }

    static Ref<Project> GetActive()
    {
        return s_ActiveProject;
    }
    static void SetActive(Ref<Project> project)
    {
        s_ActiveProject = project;
    }

    static std::filesystem::path GetAssetDirectory()
    {
        if (s_ActiveProject)
            return s_ActiveProject->m_Config.ProjectDirectory /
                   s_ActiveProject->m_Config.AssetDirectory;
        return "";
    }

    void SetActiveScenePath(const std::filesystem::path &path)
    {
        m_Config.ActiveScenePath = path;
    }

    void SetName(const std::string &name)
    {
        m_Config.Name = name;
    }

private:
    ProjectConfig m_Config;
    static Ref<Project> s_ActiveProject;

    friend class ProjectSerializer;
};
} // namespace CH

#endif // CH_PROJECT_H
