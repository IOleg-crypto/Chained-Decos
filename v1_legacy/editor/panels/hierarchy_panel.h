#ifndef CD_EDITOR_PANELS_HIERARCHY_PANEL_H
#define CD_EDITOR_PANELS_HIERARCHY_PANEL_H

#include "editor/panels/editor_panel.h"
#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <string>

namespace CHEngine
{
class Scene;
class SelectionManager;
class EditorEntityFactory;
class CommandHistory;

/**
 * @brief Panel for displaying and managing scene hierarchy using Pure ECS.
 */
class HierarchyPanel : public EditorPanel
{
public:
    HierarchyPanel() = default;
    HierarchyPanel(const std::shared_ptr<Scene> &scene, SelectionManager *selection,
                   EditorEntityFactory *factory, CommandHistory *history);
    ~HierarchyPanel() = default;

    // --- Panel Lifecycle ---
public:
    virtual void OnImGuiRender() override;

    // --- Configuration & Context ---
public:
    void SetContext(const std::shared_ptr<Scene> &scene);

    // --- Member Variables ---
private:
    std::shared_ptr<Scene> m_Context;

    SelectionManager *m_SelectionManager = nullptr;
    EditorEntityFactory *m_EntityFactory = nullptr;
    CommandHistory *m_CommandHistory = nullptr;
};
} // namespace CHEngine

#endif // CD_EDITOR_PANELS_HIERARCHY_PANEL_H
