#ifndef HIERARCHYPANEL_H
#define HIERARCHYPANEL_H

#include "editor/panels/EditorPanel.h"
#include <entt/entt.hpp>
#include <functional>
#include <memory>
#include <string>

namespace CHEngine
{
class GameScene;
class Scene;
class SelectionManager;
class EditorEntityFactory;
class CommandHistory;

/**
 * @brief Panel for displaying and managing scene hierarchy
 */
class HierarchyPanel : public EditorPanel
{
public:
    HierarchyPanel() = default;
    HierarchyPanel(const std::shared_ptr<GameScene> &scene, SelectionManager *selection,
                   EditorEntityFactory *factory, CommandHistory *history);
    ~HierarchyPanel() = default;

    // --- Panel Lifecycle ---
public:
    virtual void OnImGuiRender() override;

    // --- Configuration & Context ---
public:
    void SetContext(const std::shared_ptr<GameScene> &scene);
    void SetSceneContext(const std::shared_ptr<Scene> &scene);

    bool IsVisible() const;
    void SetVisible(bool visible);

    using SceneContextCallback = std::function<void(int)>; // 0 = Game, 1 = UI
    void SetSceneContextChangedCallback(const SceneContextCallback &callback);

    // --- Member Variables ---
private:
    std::shared_ptr<GameScene> m_Context;
    std::shared_ptr<Scene> m_SceneContext;

    SelectionManager *m_SelectionManager = nullptr;
    EditorEntityFactory *m_EntityFactory = nullptr;
    CommandHistory *m_CommandHistory = nullptr;

    SceneContextCallback m_OnContextChanged;
    int m_CurrentContextIndex = 0;
};
} // namespace CHEngine

#endif // HIERARCHYPANEL_H
