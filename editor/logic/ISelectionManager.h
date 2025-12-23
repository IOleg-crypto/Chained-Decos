#ifndef ISELECTION_MANAGER_H
#define ISELECTION_MANAGER_H

#include <scene/resources/map/core/MapData.h>

class ISelectionManager
{
public:
    virtual ~ISelectionManager() = default;

    // 3D Object Selection
    virtual MapObjectData *GetSelectedObject() = 0;
    virtual int GetSelectedObjectIndex() const = 0;
    virtual void SelectObject(int index) = 0;
    virtual void ClearSelection() = 0;

    // UI Selection
    virtual void SelectUIElement(int index) = 0;
    virtual int GetSelectedUIElementIndex() const = 0;
    virtual void RefreshUIEntities() = 0;
};

#endif // ISELECTION_MANAGER_H
