#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include "EditorSettings.h"
#include "IProjectManager.h"
#include <string>
#include <vector>

class IEditor;

class ProjectManager : public IProjectManager
{
public:
    ProjectManager(IEditor *editor);
    ~ProjectManager() override;

    const std::string &GetProjectPath() const override;
    void SetProjectPath(const std::string &path) override;
    bool CreateNewProject(const std::string &path) override;
    void SaveProject() override;
    void LoadProject(const std::string &path) override;

    const std::vector<std::string> &GetRecentProjects() const override;
    void AddRecentProject(const std::string &path) override;

    void ExportBuildManifest() override;

private:
    IEditor *m_editor;
    std::string m_projectPath;
    std::string m_settingsPath;
    EditorSettings m_editorSettings;
};

#endif // PROJECT_MANAGER_H
