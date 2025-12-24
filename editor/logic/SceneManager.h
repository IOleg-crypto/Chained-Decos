#ifndef SCENE_MANAGER_IMPL_H
#define SCENE_MANAGER_IMPL_H

#include "ISceneManager.h"
#include "MapManager.h"
#include "Scene.h"
#include <memory>

class SceneManager : public ISceneManager
{
public:
    SceneManager();
    ~SceneManager() override = default;

    // Scene Lifecycle
    void ClearScene() override;
    void SaveScene(const std::string &path = "") override;
    void LoadScene(const std::string &path) override;
    CHEngine::Scene *GetActiveScene() override
    {
        return m_activeScene.get();
    }
    GameScene &GetGameScene() override;
    void RemoveObject(int index) override;
    void RefreshUIEntities() override;
    void RefreshMapEntities() override;
    void SyncEntitiesToMap() override;

    // Scene State
    bool IsSceneModified() const override
    {
        return m_modified;
    }
    void SetSceneModified(bool modified) override
    {
        m_modified = modified;
    }
    const std::string &GetCurrentMapPath() const override
    {
        return m_currentMapPath;
    }

    // Environment/Metadata
    void SetSkybox(const std::string &name) override;
    void SetSkyboxTexture(const std::string &texturePath) override;
    void SetSkyboxColor(Color color) override;
    Skybox *GetSkybox() const override
    {
        return m_skybox.get();
    }
    Color GetClearColor() const override
    {
        return m_clearColor;
    }
    void ApplyMetadata(const MapMetadata &metadata) override;

    // Spawning/Entities (High level)
    void CreateDefaultObject(MapObjectType type, const std::string &modelName = "") override;
    void LoadAndSpawnModel(const std::string &path) override;

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
