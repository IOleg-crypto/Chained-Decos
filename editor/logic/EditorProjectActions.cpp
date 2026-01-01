#include "EditorProjectActions.h"
#include "ProjectManager.h"
#include "core/Base.h"
#include "core/Log.h"
#include "editor/panels/ContentBrowserPanel.h"
#include "editor/panels/ProjectBrowserPanel.h"


namespace CHEngine
{

EditorProjectActions::EditorProjectActions(ProjectManager *projectManager,
                                           ContentBrowserPanel *contentBrowser,
                                           ProjectBrowserPanel *projectBrowser,
                                           bool *showProjectBrowser)
    : m_ProjectManager(projectManager), m_ContentBrowser(contentBrowser),
      m_ProjectBrowser(projectBrowser), m_ShowProjectBrowser(showProjectBrowser)
{
}

void EditorProjectActions::NewProject(const std::string &name, const std::string &location)
{
    if (!m_ProjectManager)
        return;

    auto activeProject = m_ProjectManager->NewProject(name, location);
    if (activeProject)
    {
        if (m_ShowProjectBrowser)
            *m_ShowProjectBrowser = false;
        if (m_ProjectBrowser)
            m_ProjectBrowser->AddRecentProject(activeProject->GetProjectFilePath().string());

        // Update asset browser to point to project assets
        if (m_ContentBrowser)
        {
            m_ContentBrowser->SetRootDirectory(activeProject->GetAssetDirectory());
        }

        CD_INFO("[EditorProjectActions] Created new project: %s", name.c_str());
    }
    else
    {
        CD_ERROR("[EditorProjectActions] Failed to create project: %s", name.c_str());
    }
}

void EditorProjectActions::OpenProject(const std::string &projectPath)
{
    if (!m_ProjectManager)
        return;

    auto activeProject = m_ProjectManager->OpenProject(projectPath);
    if (activeProject)
    {
        if (m_ShowProjectBrowser)
            *m_ShowProjectBrowser = false;
        if (m_ProjectBrowser)
            m_ProjectBrowser->AddRecentProject(projectPath);

        // Update asset browser
        if (m_ContentBrowser)
        {
            m_ContentBrowser->SetRootDirectory(activeProject->GetAssetDirectory());
        }

        CD_INFO("[EditorProjectActions] Opened project: %s", activeProject->GetName().c_str());
    }
    else
    {
        CD_ERROR("[EditorProjectActions] Failed to open project: %s", projectPath.c_str());
    }
}

void EditorProjectActions::CloseProject()
{
    if (!m_ProjectManager)
        return;

    m_ProjectManager->CloseProject();
    if (m_ShowProjectBrowser)
        *m_ShowProjectBrowser = true;
}

void EditorProjectActions::SaveProject()
{
    if (m_ProjectManager)
    {
        m_ProjectManager->SaveProject();
    }
}

} // namespace CHEngine
