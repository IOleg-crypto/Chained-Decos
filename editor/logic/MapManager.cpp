#include "editor/logic/MapManager.h"
#include "scene/resources/map/core/MapLoader.h"
#include "scene/resources/map/mapfilemanager/json/jsonMapFileManager.h"
#include <algorithm>

void MapManager::SaveMap(const std::string &filename)
{
    std::string savePath = filename;
    if (savePath.empty())
    {
        savePath = m_currentMapPath;
    }

    if (savePath.empty())
    {
        TraceLog(LOG_WARNING,
                 "[MapManager] Cannot save map: No filename provided and no current map active.");
        return;
    }

    MapLoader loader;
    if (loader.SaveMap(m_gameMap, savePath))
    {
        m_currentMapPath = savePath;
        m_isSceneModified = false;
        TraceLog(LOG_INFO, "[MapManager] Saved map to: %s", savePath.c_str());
    }
    else
    {
        TraceLog(LOG_ERROR, "[MapManager] FAILED to save map to: %s", savePath.c_str());
    }
}

void MapManager::LoadMap(const std::string &filename)
{
    MapLoader loader;
    auto map = loader.LoadMap(filename);

    // Move the loaded map into our member
    m_gameMap = std::move(map);
    m_currentMapPath = filename;
    m_isSceneModified = false;
    m_selectedIndex = -1;
}

void MapManager::ClearScene()
{
    m_gameMap = GameMap();
    m_selectedIndex = -1;
    m_isSceneModified = false;
    m_currentMapPath = "";
}

void MapManager::AddObject(const MapObjectData &obj)
{
    m_gameMap.GetMapObjectsMutable().push_back(obj);
    m_isSceneModified = true;
    m_selectedIndex = static_cast<int>(m_gameMap.GetMapObjects().size()) - 1;
}

void MapManager::RemoveObject(int index)
{
    auto &objects = m_gameMap.GetMapObjectsMutable();
    if (index >= 0 && index < static_cast<int>(objects.size()))
    {
        objects.erase(objects.begin() + index);
        m_isSceneModified = true;
        if (m_selectedIndex == index)
            m_selectedIndex = -1;
        else if (m_selectedIndex > index)
            m_selectedIndex--;
    }
}

void MapManager::SelectObject(int index)
{
    if (index >= 0 && index < static_cast<int>(m_gameMap.GetMapObjects().size()))
    {
        m_selectedIndex = index;
    }
    else
    {
        m_selectedIndex = -1;
    }
}

void MapManager::ClearSelection()
{
    m_selectedIndex = -1;
}

void MapManager::ClearObjects()
{
    m_gameMap.GetMapObjectsMutable().clear();
    m_isSceneModified = true;
    m_selectedIndex = -1;
}

MapObjectData *MapManager::GetSelectedObject()
{
    if (m_selectedIndex >= 0 &&
        m_selectedIndex < static_cast<int>(m_gameMap.GetMapObjects().size()))
    {
        return &m_gameMap.GetMapObjectsMutable()[m_selectedIndex];
    }
    return nullptr;
}
GameMap &MapManager::GetGameMap()
{
    return m_gameMap;
}
int MapManager::GetSelectedIndex() const
{
    return m_selectedIndex;
}
bool MapManager::IsSceneModified() const
{
    return m_isSceneModified;
}
void MapManager::SetSceneModified(bool modified)
{
    m_isSceneModified = modified;
}
const std::string &MapManager::GetCurrentMapPath() const
{
    return m_currentMapPath;
}
