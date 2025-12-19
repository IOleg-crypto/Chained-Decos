#ifndef MAP_MANAGER_H
#define MAP_MANAGER_H

#include "scene/resources/map/Core/MapLoader.h"
#include <string>

class MapManager
{
public:
    MapManager() = default;
    ~MapManager() = default;

    // File operations
    void SaveMap(const std::string &filename);
    void LoadMap(const std::string &filename);
    void ClearScene();

    // Object management
    void AddObject(const MapObjectData &obj);
    void RemoveObject(int index);
    void SelectObject(int index);
    void ClearSelection();
    void ClearObjects();

    // Accessors
    GameMap &GetGameMap();
    int GetSelectedIndex() const;
    MapObjectData *GetSelectedObject();
    bool IsSceneModified() const;
    void SetSceneModified(bool modified);
    const std::string &GetCurrentMapPath() const;

private:
    GameMap m_gameMap;
    int m_selectedIndex = -1;
    bool m_isSceneModified = false;
    std::string m_currentMapPath;
};

#endif // MAP_MANAGER_H
