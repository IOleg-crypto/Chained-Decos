#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "ISceneManager.h"
#include "../Object/MapObject.h"
#include <vector>
#include <memory>

class MapObject;
class CollisionRay;

// Concrete implementation of scene management subsystem
class SceneManager : public ISceneManager {
private:
    std::vector<MapObject> m_objects;  // Collection of scene objects
    int m_selectedIndex;               // Index of currently selected object (-1 if none)

public:
    SceneManager();
    ~SceneManager() override = default;

    // Object management
    void AddObject(const MapObject& obj) override;
    void RemoveObject(int index) override;
    void SelectObject(int index) override;
    void ClearSelection() override;

    // Object access
    MapObject* GetSelectedObject() override;
    const std::vector<MapObject>& GetObjects() const override;
    int GetSelectedObjectIndex() const override;

    // Object picking
    int PickObject(const CollisionRay& ray) override;

private:
    // Helper methods
    bool IsValidIndex(int index) const;
    void UpdateSelectionIndices(int removedIndex);
};

#endif // SCENEMANAGER_H