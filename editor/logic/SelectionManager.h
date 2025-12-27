#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include "editor/EditorTypes.h"
#include "scene/resources/map/GameScene.h"
#include <entt/entt.hpp>
#include <memory>

namespace CHEngine
{
class SelectionManager
{
public:
    SelectionManager() = default;

    void SetSelection(int index, SelectionType type = SelectionType::WORLD_OBJECT)
    {
        m_SelectedIndex = index;
        m_SelectionType = type;
        m_SelectedEntity = entt::null;
    }

    void SetEntitySelection(entt::entity entity)
    {
        m_SelectedEntity = entity;
        m_SelectionType = SelectionType::ENTITY;
        m_SelectedIndex = -1;
    }

    void ClearSelection()
    {
        m_SelectedIndex = -1;
        m_SelectionType = SelectionType::NONE;
        m_SelectedEntity = entt::null;
    }

    int GetSelectedIndex() const
    {
        return m_SelectedIndex;
    }
    SelectionType GetSelectionType() const
    {
        return m_SelectionType;
    }

    entt::entity GetSelectedEntity() const
    {
        return m_SelectedEntity;
    }

    int GetSelectedObjectIndex() const
    {
        return (m_SelectionType == SelectionType::WORLD_OBJECT) ? m_SelectedIndex : -1;
    }

    MapObjectData *GetSelectedObject(const std::shared_ptr<GameScene> &scene);
    UIElementData *GetSelectedUIElement(const std::shared_ptr<GameScene> &scene);

private:
    int m_SelectedIndex = -1;
    SelectionType m_SelectionType = SelectionType::NONE;
    entt::entity m_SelectedEntity = entt::null;
};
} // namespace CHEngine

#endif // SELECTION_MANAGER_H
