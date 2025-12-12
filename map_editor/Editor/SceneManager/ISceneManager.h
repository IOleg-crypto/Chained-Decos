#ifndef ISCENEMANAGER_H
#define ISCENEMANAGER_H

#include "../Object/MapObject.h"
#include "components/physics/collision/Structures/CollisionStructures.h"
#include <memory>
#include <vector>

class GameObject; // Forward declaration

// Interface for scene management subsystem
class ISceneManager
{
public:
    virtual ~ISceneManager() = default;

    // Object management
    virtual void AddObject(const MapObject &obj) = 0;
    virtual void RemoveObject(int index) = 0;
    virtual void SelectObject(int index) = 0;
    virtual void ClearSelection() = 0;
    virtual void ClearScene() = 0;

    // Modification Tracking
    virtual bool IsSceneModified() const = 0;
    virtual void SetSceneModified(bool modified) = 0;

    // ECS Object Management
    virtual void AddGameObject(std::unique_ptr<GameObject> obj) = 0;
    virtual void RemoveGameObject(GameObject *obj) = 0;
    virtual const std::vector<std::unique_ptr<GameObject>> &GetGameObjects() const = 0;

    // ECS Selection
    virtual void SelectGameObject(GameObject *obj) = 0;
    virtual GameObject *GetSelectedGameObject() const = 0;

    // Object access
    virtual MapObject *GetSelectedObject() = 0;
    virtual const std::vector<MapObject> &GetObjects() const = 0;
    virtual int GetSelectedObjectIndex() const = 0;

    // Object picking
    virtual int PickObject(const CollisionRay &ray) = 0;
};

#endif // ISCENEMANAGER_H