#pragma once
#include "editor_panel.h"
#include <functional>
#include <string>
#include <vector>

namespace CHEngine
{
class EditorProjectActions;

/**
 * @brief Welcome screen panel for creating/opening projects
 */
class ProjectBrowserPanel : public EditorPanel
{
public:
    ProjectBrowserPanel(EditorProjectActions *projectActions);
    virtual ~ProjectBrowserPanel() = default;

    // --- Panel Lifecycle ---
public:
    virtual void OnImGuiRender() override;

    // --- Configuration ---
public:
    bool IsVisible() const;
    void SetVisible(bool visible);
    void OpenCreateDialog();

    // --- Callbacks ---
public:
    void SetOnCreateProject(std::function<void(const std::string &, const std::string &)> callback);
    void SetOnOpenProject(std::function<void(const std::string &)> callback);

    // --- Recent Projects Management ---
public:
    void AddRecentProject(const std::string &path);
    const std::vector<std::string> &GetRecentProjects() const;

    // --- Private Drawing Helpers ---
private:
    void DrawWelcomeScreen();
    void DrawCreateProjectDialog();
    void DrawRecentProjects();

    // --- Member Variables ---
private:
    EditorProjectActions *m_ProjectActions = nullptr;

    std::vector<std::string> m_RecentProjects;

    // UI State
    bool m_ShowCreateDialog = false;
    bool m_TriggerOpenCreateDialog = false;
    char m_ProjectNameBuffer[256] = "MyProject";
    char m_ProjectLocationBuffer[512] = "";
};
} // namespace CHEngine
