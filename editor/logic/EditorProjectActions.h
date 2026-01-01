#ifndef EDITOR_PROJECT_ACTIONS_H
#define EDITOR_PROJECT_ACTIONS_H

#include <memory>
#include <string>

namespace CHEngine
{
class ProjectManager;
class ContentBrowserPanel;
class ProjectBrowserPanel;

class EditorProjectActions
{
public:
    EditorProjectActions(ProjectManager *projectManager, ContentBrowserPanel *contentBrowser,
                         ProjectBrowserPanel *projectBrowser, bool *showProjectBrowser);
    ~EditorProjectActions() = default;

    void NewProject(const std::string &name, const std::string &location);
    void OpenProject(const std::string &projectPath);
    void CloseProject();
    void SaveProject();

private:
    ProjectManager *m_ProjectManager;
    ContentBrowserPanel *m_ContentBrowser;
    ProjectBrowserPanel *m_ProjectBrowser;
    bool *m_ShowProjectBrowser;
};
} // namespace CHEngine

#endif // EDITOR_PROJECT_ACTIONS_H
