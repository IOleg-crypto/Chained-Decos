#ifndef EDITOR_SCENE_MANAGER_H
#define EDITOR_SCENE_MANAGER_H

#include "EditorMapManager.h"
#include "scene/core/Scene.h"
#include "scene/resources/map/GameScene.h"
#include <memory>
#include <string>

namespace CHEngine
{
class EditorSceneManager
{
public:
    EditorSceneManager();
    ~EditorSceneManager() = default;

    // Scene Lifecycle
    void ClearScene();
    void SaveScene(const std::string &path = "");
    void LoadScene(const std::string &path);
    CHEngine::Scene *GetActiveScene()
    {
        return m_activeScene.get();
    }
    CHEngine::Scene *GetUIScene()
    {
        return m_uiScene.get();
    }
    CHEngine::GameScene &GetGameScene();

    enum class SceneContext
    {
        Game,
        UI
    };
    void SetContext(SceneContext context)
    {
        m_currentContext = context;
    }
    SceneContext GetContext() const
    {
        return m_currentContext;
    }
    CHEngine::Scene *GetCurrentEditingScene()
    {
        return m_currentContext == SceneContext::Game ? m_activeScene.get() : m_uiScene.get();
    }

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
    std::unique_ptr<CHEngine::Scene> m_uiScene;
    SceneContext m_currentContext = SceneContext::Game;

    std::string m_currentMapPath;
    bool m_modified = false;
    std::unique_ptr<Skybox> m_skybox;
    Color m_clearColor = DARKGRAY;
    // editor-specific map manager
    std::unique_ptr<EditorMapManager> m_mapManager;
};
} // namespace CHEngine

#endif // EDITOR_SCENE_MANAGER_H
