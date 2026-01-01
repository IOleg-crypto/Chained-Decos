#ifndef EDITOR_MAP_MANAGER_H
#define EDITOR_MAP_MANAGER_H

#include "scene/resources/map/GameScene.h"
#include "scene/resources/map/SceneLoader.h"
#include <string>
#include <vector>

namespace CHEngine
{
class GameScene;
}

namespace CHEngine
{
class EditorMapManager
{
public:
    EditorMapManager() = default;
    ~EditorMapManager() = default;

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
    CHEngine::GameScene &GetGameScene();
    int GetSelectedIndex() const;
    MapObjectData *GetSelectedObject();
    bool IsSceneModified() const;
    void SetSceneModified(bool modified);
    const std::string &GetCurrentMapPath() const;

private:
    CHEngine::GameScene m_gameScene;
    int m_selectedIndex = -1;
    bool m_isSceneModified = false;
    std::string m_currentMapPath;
};
} // namespace CHEngine

#endif // EDITOR_MAP_MANAGER_H
