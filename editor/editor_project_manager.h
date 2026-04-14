#ifndef CH_EDITOR_PROJECT_MANAGER_H
#define CH_EDITOR_PROJECT_MANAGER_H

#include "engine/scene/scene.h"
#include "engine/scene/scene_events.h"
#include "editor_context.h"
#include "engine/core/base.h"
#include "editor_events.h"
#include <filesystem>
#include <string>

namespace CHEngine
{

class EditorProjectManager
{
public:
    EditorProjectManager();
    ~EditorProjectManager() = default;

    void NewProject();
    void NewProject(const std::string& name, const std::string& path);
    void OpenProject();
    void OpenProject(const std::filesystem::path& path);
    void SaveProject();

    bool OnProjectOpened(ProjectOpenedEvent& e);

    const std::string& GetLastProjectPath() const { return m_LastProjectPath; }
    void SetLastProjectPath(const std::string& path) { m_LastProjectPath = path; }

private:
    std::string m_LastProjectPath;
};

} // namespace CHEngine

#endif // CH_EDITOR_PROJECT_MANAGER_H
