#ifndef SCENE_MANAGER_IMPL_H
#define SCENE_MANAGER_IMPL_H

#include "MapManager.h"
#include "scene/core/Scene.h"
#include <memory>
#include <string>

class SceneManager
{
public:
    SceneManager();
    ~SceneManager() = default;

    // Scene Lifecycle
    void ClearScene();
    void SaveScene(const std::string &path = "");
    void LoadScene(const std::string &path);
    CHEngine::Scene *GetActiveScene()
    {
        return m_activeScene.get();
    }
    GameScene &GetGameScene();
    void RemoveObject(int index);
    void RefreshUIEntities();
    void RefreshMapEntities();
    void SyncEntitiesToMap();

    // Scene State
    bool IsSceneModified() const
    {
        return m_modified;
    }
    void SetSceneModified(bool modified)
    {
        m_modified = modified;
    }
    const std::string &GetCurrentMapPath() const
    {
        return m_currentMapPath;
    }

    // Environment/Metadata
    void SetSkybox(const std::string &name);
    void SetSkyboxTexture(const std::string &texturePath);
    void SetSkyboxColor(Color color);
    Skybox *GetSkybox() const
    {
        return m_skybox.get();
    }
    Color GetClearColor() const
    {
        return m_clearColor;
    }
    void ApplyMetadata(const MapMetadata &metadata);

    // Spawning/Entities (High level)
    void CreateDefaultObject(MapObjectType type, const std::string &modelName = "");
    void LoadAndSpawnModel(const std::string &path);

private:
    std::unique_ptr<CHEngine::Scene> m_activeScene;
    std::string m_currentMapPath;
    bool m_modified = false;
    std::unique_ptr<Skybox> m_skybox;
    Color m_clearColor = DARKGRAY;
    // Legacy mapping support
    std::unique_ptr<MapManager> m_mapManager;
};

#endif // SCENE_MANAGER_IMPL_H
