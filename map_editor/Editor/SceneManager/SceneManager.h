#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include "../ECS/GameObject.h"
#include "../Object/MapObject.h"
#include "ISceneManager.h"
#include "components/physics/collision/Structures/CollisionStructures.h"
#include <memory>
#include <vector>

// Concrete implementation of scene management subsystem
class SceneManager : public ISceneManager
{
private:
    std::vector<MapObject> m_objects;                       // Collection of scene objects
    std::vector<std::unique_ptr<GameObject>> m_gameObjects; // ECS GameObjects
    int m_selectedIndex;              // Index of currently selected object (-1 if none)
    GameObject *m_selectedGameObject; // Pointer to selected ECS object (nullptr if none)

    bool m_isSceneModified = false;

public:
    SceneManager();
    ~SceneManager() override = default;

    // Object management
    void AddObject(const MapObject &obj) override;
    void RemoveObject(int index) override;
    void SelectObject(int index) override;
    void ClearSelection() override;
    void ClearScene() override;

    // Modification Tracking
    bool IsSceneModified() const override;
    void SetSceneModified(bool modified) override;

    // ECS Object Management
    void AddGameObject(std::unique_ptr<GameObject> obj) override;
    void RemoveGameObject(GameObject *obj) override;
    const std::vector<std::unique_ptr<GameObject>> &GetGameObjects() const override;

    // ECS Selection
    void SelectGameObject(GameObject *obj) override;
    GameObject *GetSelectedGameObject() const override;

    // Object access
    MapObject *GetSelectedObject() override;
    const std::vector<MapObject> &GetObjects() const override;
    int GetSelectedObjectIndex() const override;

    // Object picking
    int PickObject(const CollisionRay &ray) override;

private:
    // Helper methods
    bool IsValidIndex(int index) const;
    void UpdateSelectionIndices(int removedIndex);
};

#endif // SCENEMANAGER_H