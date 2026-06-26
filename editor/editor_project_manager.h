#ifndef CH_EDITOR_PROJECT_MANAGER_H
#define CH_EDITOR_PROJECT_MANAGER_H


#include "editor/project/editor_settings.h"
#include "engine/scene/scene_events.h"
#include <filesystem>
#include <string>


namespace Chained
{
class EditorProjectManager
{
public:
    EditorProjectManager();
    ~EditorProjectManager() = default;

public:
    void NewProject();
    void NewProject(const std::string& name, const std::string& path);
    void OpenProject();
    void OpenProject(const std::filesystem::path& path);
    void SaveProject();

    bool OnProjectOpened(ProjectOpenedEvent& e);

    const std::string& GetLastProjectPath() const;

    void SetLastProjectPath(const std::string& path);
public:
    EditorSettings& GetEditorSettings() { return m_EditorSettings; }
    const EditorSettings& GetEditorSettings() const { return m_EditorSettings; }

private:
    std::string m_LastProjectPath;
    EditorSettings m_EditorSettings;
};

} // namespace Chained

#endif // CH_EDITOR_PROJECT_MANAGER_H
