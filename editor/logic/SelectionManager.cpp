#include "SelectionManager.h"

namespace CHEngine
{
MapObjectData *SelectionManager::GetSelectedObject(const std::shared_ptr<GameScene> &scene)
{
    if (m_SelectionType != SelectionType::WORLD_OBJECT || m_SelectedIndex < 0 || !scene)
        return nullptr;

    auto &objects = scene->GetMapObjectsMutable();
    if (m_SelectedIndex >= (int)objects.size())
        return nullptr;

    return &objects[m_SelectedIndex];
}

UIElementData *SelectionManager::GetSelectedUIElement(const std::shared_ptr<GameScene> &scene)
{
    if (m_SelectionType != SelectionType::UI_ELEMENT || m_SelectedIndex < 0 || !scene)
        return nullptr;

    auto &elements = scene->GetUIElementsMutable();
    if (m_SelectedIndex >= (int)elements.size())
        return nullptr;

    return &elements[m_SelectedIndex];
}
} // namespace CHEngine
