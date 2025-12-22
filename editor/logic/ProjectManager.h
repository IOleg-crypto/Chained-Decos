#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include "EditorSettings.h"
#include <string>
#include <vector>


class IProjectManager
{
public:
    virtual ~IProjectManager() = default;

    virtual const std::string &GetProjectPath() const = 0;
    virtual void SetProjectPath(const std::string &path) = 0;
    virtual bool CreateNewProject(const std::string &path) = 0;
    virtual void SaveProject() = 0;
    virtual void LoadProject(const std::string &path) = 0;

    virtual const std::vector<std::string> &GetRecentProjects() const = 0;
    virtual void AddRecentProject(const std::string &path) = 0;
};

class ProjectManager : public IProjectManager
{
public:
    ProjectManager();
    ~ProjectManager() override;

    const std::string &GetProjectPath() const override
    {
        return m_projectPath;
    }
    void SetProjectPath(const std::string &path) override;
    bool CreateNewProject(const std::string &path) override;
    void SaveProject() override;
    void LoadProject(const std::string &path) override;

    const std::vector<std::string> &GetRecentProjects() const override;
    void AddRecentProject(const std::string &path) override;

private:
    std::string m_projectPath;
    std::string m_settingsPath;
    EditorSettings m_editorSettings;
};

#endif // PROJECT_MANAGER_H
