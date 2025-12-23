#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include "ISelectionManager.h"
#include <vector>

namespace CHEngine
{
class Scene;
}

class SelectionManager : public ISelectionManager
{
public:
    SelectionManager() = default;
    ~SelectionManager() override = default;

    // 3D Object Selection
    MapObjectData *GetSelectedObject() override;
    int GetSelectedObjectIndex() const override
    {
        return m_selectedObjectIndex;
    }
    void SelectObject(int index) override
    {
        m_selectedObjectIndex = index;
    }
    void ClearSelection() override
    {
        m_selectedObjectIndex = -1;
        m_selectedUIElementIndex = -1;
    }

    // UI Selection
    void SelectUIElement(int index) override
    {
        m_selectedUIElementIndex = index;
    }
    int GetSelectedUIElementIndex() const override
    {
        return m_selectedUIElementIndex;
    }
    void RefreshUIEntities() override; // May need external context later

private:
    int m_selectedObjectIndex = -1;
    int m_selectedUIElementIndex = -1;
};

#endif // SELECTION_MANAGER_H
