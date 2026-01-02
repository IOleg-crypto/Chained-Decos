#ifndef CD_EDITOR_PANELS_INSPECTOR_PANEL_H
#define CD_EDITOR_PANELS_INSPECTOR_PANEL_H

#include "editor/panels/editor_panel.h"
#include <entt/entt.hpp>
#include <memory>
#include <string>

namespace CHEngine
{
class Scene;
class SelectionManager;
class CommandHistory;
class EditorSceneManager;
class EditorSceneActions;

/**
 * @brief Panel for inspecting and editing entity properties in Pure ECS.
 */
class InspectorPanel : public EditorPanel
{
public:
    InspectorPanel() = default;
    InspectorPanel(SelectionManager *selection, CommandHistory *history,
                   EditorSceneManager *sceneManager, EditorSceneActions *sceneActions);
    ~InspectorPanel() = default;

    // --- Panel Lifecycle ---
public:
    virtual void OnImGuiRender() override;

private:
    void DrawEntityComponents(const std::shared_ptr<Scene> &scene, entt::entity entity);

    // --- Member Variables ---
private:
    SelectionManager *m_SelectionManager = nullptr;
    CommandHistory *m_CommandHistory = nullptr;
    EditorSceneManager *m_SceneManager = nullptr;
    EditorSceneActions *m_SceneActions = nullptr;
};
} // namespace CHEngine

#endif // CD_EDITOR_PANELS_INSPECTOR_PANEL_H
