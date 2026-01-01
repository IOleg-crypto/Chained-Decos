#include "EditorMapManager.h"
#include "core/Log.h"
#include "scene/resources/map/SceneLoader.h"

namespace CHEngine
{

void EditorMapManager::SaveScene(const std::string &filename)
{
    std::string savePath = filename;
    if (savePath.empty())
    {
        savePath = m_currentMapPath;
    }

    if (savePath.empty())
    {
        CD_CORE_WARN(
            "[EditorMapManager] Cannot save map: No filename provided and no current map active.");
        return;
    }

    SceneLoader loader;
    if (loader.SaveScene(m_gameScene, savePath))
    {
        m_currentMapPath = savePath;
        m_isSceneModified = false;
        CD_CORE_INFO("[EditorMapManager] Saved map to: %s", savePath.c_str());
    }
    else
    {
        CD_CORE_ERROR("[EditorMapManager] FAILED to save map to: %s", savePath.c_str());
    }
}

void EditorMapManager::LoadScene(const std::string &filename)
{
    SceneLoader loader;
    auto map = loader.LoadScene(filename);

    // Move the loaded map into our member
    m_gameScene = std::move(map);
    m_currentMapPath = filename;
    m_isSceneModified = false;
    m_selectedIndex = -1;
}

void EditorMapManager::ClearScene()
{
    m_gameScene = GameScene();
    m_selectedIndex = -1;
    m_isSceneModified = false;
    m_currentMapPath = "";
}

void EditorMapManager::AddObject(const MapObjectData &obj)
{
    m_gameScene.GetMapObjectsMutable().push_back(obj);
    m_isSceneModified = true;
    m_selectedIndex = static_cast<int>(m_gameScene.GetMapObjects().size()) - 1;
}

void EditorMapManager::RemoveObject(int index)
{
    auto &objects = m_gameScene.GetMapObjectsMutable();
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

void EditorMapManager::SelectObject(int index)
{
    if (index >= 0 && index < static_cast<int>(m_gameScene.GetMapObjects().size()))
    {
        m_selectedIndex = index;
    }
    else
    {
        m_selectedIndex = -1;
    }
}

void EditorMapManager::ClearSelection()
{
    m_selectedIndex = -1;
}

void EditorMapManager::ClearObjects()
{
    m_gameScene.GetMapObjectsMutable().clear();
    m_isSceneModified = true;
    m_selectedIndex = -1;
}

MapObjectData *EditorMapManager::GetSelectedObject()
{
    if (m_selectedIndex >= 0 &&
        m_selectedIndex < static_cast<int>(m_gameScene.GetMapObjects().size()))
    {
        return &m_gameScene.GetMapObjectsMutable()[m_selectedIndex];
    }
    return nullptr;
}

GameScene &EditorMapManager::GetGameScene()
{
    return m_gameScene;
}

int EditorMapManager::GetSelectedIndex() const
{
    return m_selectedIndex;
}

bool EditorMapManager::IsSceneModified() const
{
    return m_isSceneModified;
}

void EditorMapManager::SetSceneModified(bool modified)
{
    m_isSceneModified = modified;
}

const std::string &EditorMapManager::GetCurrentMapPath() const
{
    return m_currentMapPath;
}

} // namespace CHEngine
