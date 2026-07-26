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
    void LaunchStandalone(std::shared_ptr<Scene> editorScene);

    bool OnProjectOpened(ProjectOpenedEvent& e);

    // Runs the deferred part of project opening (font atlas rebuild, scene load).
    // Must be called outside the ImGui frame — see EditorLayer::OnUpdate().
    void ProcessPendingProjectOpen();

    const std::string& GetLastProjectPath() const;

    void SetLastProjectPath(const std::string& path);

private:
    std::string m_LastProjectPath;
    std::string m_PendingOpenedProjectPath;
};

} // namespace Chained

#endif // CH_EDITOR_PROJECT_MANAGER_H
