#ifndef ISCENEMANAGER_H
#define ISCENEMANAGER_H

#include <vector>
#include <memory>
#include "../Object/MapObject.h"
#include "components/physics/collision/Structures/CollisionStructures.h"

// Interface for scene management subsystem
class ISceneManager {
public:
    virtual ~ISceneManager() = default;

    // Object management
    virtual void AddObject(const MapObject& obj) = 0;
    virtual void RemoveObject(int index) = 0;
    virtual void SelectObject(int index) = 0;
    virtual void ClearSelection() = 0;

    // Object access
    virtual MapObject* GetSelectedObject() = 0;
    virtual const std::vector<MapObject>& GetObjects() const = 0;
    virtual int GetSelectedObjectIndex() const = 0;

    // Object picking
    virtual int PickObject(const CollisionRay& ray) = 0;
};

#endif // ISCENEMANAGER_H