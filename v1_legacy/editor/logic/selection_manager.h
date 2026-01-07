#ifndef CD_EDITOR_LOGIC_SELECTION_MANAGER_H
#define CD_EDITOR_LOGIC_SELECTION_MANAGER_H

#include "editor/editor_types.h"
#include <entt/entt.hpp>
#include <memory>

namespace CHEngine
{
/**
 * @brief SelectionManager - Tracks the currently selected item in the editor.
 * Focused purely on ECS entities in the new architecture.
 */
class SelectionManager
{
public:
    static SelectionManager &Get();
    static void Init();
    static void Shutdown();

    SelectionManager() = default;
    ~SelectionManager() = default;

    void SetSelection(entt::entity entity)
    {
        m_SelectedEntity = entity;
    }

    void ClearSelection()
    {
        m_SelectedEntity = entt::null;
    }

    entt::entity GetSelectedEntity() const
    {
        return m_SelectedEntity;
    }

    bool HasSelection() const
    {
        return m_SelectedEntity != entt::null;
    }

private:
    entt::entity m_SelectedEntity = entt::null;
};
} // namespace CHEngine

#endif // CD_EDITOR_LOGIC_SELECTION_MANAGER_H
