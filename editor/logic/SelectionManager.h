#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include "scene/resources/map/core/SceneLoader.h"
#include <vector>

namespace CHEngine
{
class Scene;
}

class SelectionManager
{
public:
    SelectionManager() = default;
    ~SelectionManager() = default;

    // 3D Object Selection
    MapObjectData *GetSelectedObject();
    int GetSelectedObjectIndex() const
    {
        return m_selectedObjectIndex;
    }
    void SelectObject(int index)
    {
        m_selectedObjectIndex = index;
    }
    void ClearSelection()
    {
        m_selectedObjectIndex = -1;
        m_selectedUIElementIndex = -1;
    }

    // UI Selection
    void SelectUIElement(int index)
    {
        m_selectedUIElementIndex = index;
    }
    int GetSelectedUIElementIndex() const
    {
        return m_selectedUIElementIndex;
    }
    void RefreshUIEntities(); // May need external context later

private:
    int m_selectedObjectIndex = -1;
    int m_selectedUIElementIndex = -1;
};

#endif // SELECTION_MANAGER_H
