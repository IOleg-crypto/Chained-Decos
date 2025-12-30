#ifndef MAP_MANAGER_H
#define MAP_MANAGER_H

#include "scene/resources/map/SceneLoader.h"
#include <string>
#include <vector>

class MapManager
{
public:
    MapManager() = default;
    ~MapManager() = default;

    // File operations
    void SaveScene(const std::string &filename);
    void LoadScene(const std::string &filename);
    void ClearScene();

    // Object management
    void AddObject(const MapObjectData &obj);
    void RemoveObject(int index);
    void SelectObject(int index);
    void ClearSelection();
    void ClearObjects();

    // Accessors
    GameScene &GetGameScene();
    int GetSelectedIndex() const;
    MapObjectData *GetSelectedObject();
    bool IsSceneModified() const;
    void SetSceneModified(bool modified);
    const std::string &GetCurrentMapPath() const;

private:
    GameScene m_gameScene;
    int m_selectedIndex = -1;
    bool m_isSceneModified = false;
    std::string m_currentMapPath;
};

#endif // MAP_MANAGER_H
