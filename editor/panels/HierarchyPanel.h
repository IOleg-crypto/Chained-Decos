#ifndef HIERARCHYPANEL_H
#define HIERARCHYPANEL_H

#include "editor/EditorTypes.h"
#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <string>

class GameScene;

namespace CHEngine
{
class Scene;

/**
 * @brief Panel for displaying and managing scene hierarchy
 */
class HierarchyPanel
{
public:
    HierarchyPanel() = default;
    HierarchyPanel(const std::shared_ptr<GameScene> &scene);
    ~HierarchyPanel() = default;

    // --- Panel Lifecycle ---
public:
    void OnImGuiRender(SelectionType selectionType, int selectedIndex,
                       const std::function<void(SelectionType, int)> &onSelect,
                       const std::function<void()> &onAddModel,
                       const std::function<void(const std::string &)> &onAddUI,
                       const std::function<void(int)> &onDelete,
                       entt::entity selectedEntity = entt::null,
                       const std::function<void(entt::entity)> &onSelectEntity = nullptr,
                       const std::function<void()> &onCreateEntity = nullptr,
                       const std::function<void(entt::entity)> &onDeleteEntity = nullptr);

    // --- Configuration & Context ---
public:
    void SetContext(const std::shared_ptr<GameScene> &scene);
    void SetSceneContext(const std::shared_ptr<Scene> &scene);

    bool IsVisible() const;
    void SetVisible(bool visible);

    // --- Member Variables ---
private:
    std::shared_ptr<::GameScene> m_Context;
    std::shared_ptr<Scene> m_SceneContext;
    bool m_isVisible = true;
};
} // namespace CHEngine

#endif // HIERARCHYPANEL_H
