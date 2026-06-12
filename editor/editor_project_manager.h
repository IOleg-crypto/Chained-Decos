#ifndef CH_EDITOR_PROJECT_MANAGER_H
#define CH_EDITOR_PROJECT_MANAGER_H

#include "editor_events.h"
#include "project/editor_settings.h"
#include "engine/foundation/base.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include <filesystem>
#include <string>


namespace Chained
{

class Application;
class Renderer;
class ScriptEngine;
class UIRenderer;
class EditorLayer;

class EditorProjectManager
{
public:
    EditorProjectManager(EditorLayer& owner);
    ~EditorProjectManager() = default;

public:
    void NewProject();
    void NewProject(const std::string& name, const std::string& path);
    void OpenProject();
    void OpenProject(const std::filesystem::path& path);
    void SaveProject();

    bool OnProjectOpened(ProjectOpenedEvent& e);

    const std::string& GetLastProjectPath() const
    {
        return m_LastProjectPath;
    }
    void SetLastProjectPath(const std::string& path)
    {
        m_LastProjectPath = path;
    }

    EditorSettings& GetEditorSettings() { return m_EditorSettings; }
    const EditorSettings& GetEditorSettings() const { return m_EditorSettings; }

private:
    std::string m_LastProjectPath;
    EditorLayer& m_EditorLayer;
    EditorSettings m_EditorSettings;
};

} // namespace Chained

#endif // CH_EDITOR_PROJECT_MANAGER_H
