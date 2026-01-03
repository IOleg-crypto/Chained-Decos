#pragma once
#include "project/project.h"
#include "engine/scene/core/scene.h"
#include <functional>
#include <memory>
#include <string>

namespace CHEngine
{
/**
 * @brief Manager for project-level operations in the editor.
 *
 * Coordinates between the Project system and the Editor's active scene.
 */
class ProjectManager
{
public:
    static ProjectManager &Get();
    static void Init();
    static void Shutdown();

    ProjectManager();
    ~ProjectManager() = default;

    std::shared_ptr<Project> NewProject(const std::string &name, const std::string &location);
    std::shared_ptr<Project> OpenProject(const std::string &path);
    void SaveProject();
    void CloseProject();

    std::shared_ptr<Project> GetActiveProject() const
    {
        return m_ActiveProject;
    }

private:
    std::shared_ptr<Project> m_ActiveProject;

    static ProjectManager *s_Instance;
};
} // namespace CHEngine
