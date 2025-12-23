#ifndef IPROJECT_MANAGER_H
#define IPROJECT_MANAGER_H

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

#endif // IPROJECT_MANAGER_H
