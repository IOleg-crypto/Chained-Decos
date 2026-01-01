#ifndef LEVELMANAGER_H
#define LEVELMANAGER_H

#include "IWorldManager.h"
#include "MapCollisionInitializer.h"
#include "World.h"
#include "components/physics/collision/core/CollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/interfaces/IEngineModule.h"
#include "core/interfaces/ILevelManager.h"
#include "core/interfaces/IMenu.h"
#include "core/interfaces/IPlayer.h"
#include "scene/resources/map/SceneLoader.h"
#include "scene/resources/model/Model.h"
#include <memory>
#include <string>
#include <vector>

namespace CHEngine
{

// Configuration for LevelManager
struct LevelManagerConfig
{
    std::string resourcePath = "resources/maps";
    bool enableDebugRendering = false;
    bool enableSpawnZoneRendering = true;
};

// Central System for managing maps and levels
class LevelManager
{
public:
    static void Init(const LevelManagerConfig &config = {});
    static void Shutdown();
    static bool IsInitialized();

    static void SetActiveScene(std::shared_ptr<CHEngine::Scene> scene);
    static bool LoadScene(const std::string &path);
    static bool LoadSceneByIndex(int index);
    static bool LoadSceneByName(const std::string &name);
    static bool LoadUIScene(const std::string &path);
    static void UnloadMap();
    static void UnloadUIScene();
    static bool IsMapLoaded();
    static const std::string &GetCurrentMapPath();
    static std::string GetCurrentMapName();
    static Vector3 GetSpawnPosition();

    static void Update(float deltaTime);
    static void Render();

    // Map Management
    static void LoadEditorMap(const std::string &mapPath);
    static std::string ConvertMapNameToPath(const std::string &mapName);
    static void RenderEditorMap();
    static void RefreshMapEntities();
    static void RefreshUIEntities();
    static void SyncEntitiesToMap();
    static void RenderSpawnZone();
    static void DumpMapDiagnostics();

    // Collision Initialization
    static void InitCollisions();
    static void InitCollisionsWithModels(const std::vector<std::string> &requiredModels);
    static bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels);

    // Accessors
    static GameScene &GetGameScene();
    static Vector3 GetPlayerSpawnPosition();
    static bool HasSpawnZone();
    static MapCollisionInitializer *GetCollisionInitializer();

    static void SetPlayer(std::shared_ptr<IPlayer> player);

public:
    LevelManager(const LevelManagerConfig &config = {});
    ~LevelManager();

    void InternalShutdown();
    void InternalSetActiveScene(std::shared_ptr<CHEngine::Scene> scene);
    bool InternalLoadScene(const std::string &path);
    bool InternalLoadSceneByIndex(int index);
    bool InternalLoadSceneByName(const std::string &name);
    bool InternalLoadUIScene(const std::string &path);
    void InternalUnloadMap();
    void InternalUnloadUIScene();
    bool InternalIsMapLoaded() const;
    const std::string &InternalGetCurrentMapPath() const
    {
        return m_currentMapPath;
    }

    void InternalUpdate(float deltaTime);
    void InternalRender();

    void InternalLoadEditorMap(const std::string &mapPath);
    void InternalRenderEditorMap();
    void InternalRefreshMapEntities();
    void InternalRefreshUIEntities();
    void InternalSyncEntitiesToMap();
    void InternalRenderSpawnZone() const;
    void InternalDumpMapDiagnostics() const;

    void InternalInitCollisions();
    void InternalInitCollisionsWithModels(const std::vector<std::string> &requiredModels);
    bool InternalInitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels);

    GameScene &InternalGetGameScene()
    {
        return *m_gameScene;
    }
    std::string InternalGetCurrentMapName() const
    {
        if (m_currentMapPath.empty())
            return "";
        return std::filesystem::path(m_currentMapPath).stem().string();
    }
    Vector3 InternalGetSpawnPosition() const
    {
        return Vector3Scale(Vector3Add(m_playerSpawnZone.min, m_playerSpawnZone.max), 0.5f);
    }
    bool InternalHasSpawnZone() const
    {
        return m_hasSpawnZone;
    }
    MapCollisionInitializer *InternalGetCollisionInitializer()
    {
        return m_collisionInitializer.get();
    }

    void InternalSetPlayer(std::shared_ptr<IPlayer> player)
    {
        m_player = player;
    }

private:
    void InternalPopulateUIFromData(entt::registry &registry,
                                    const std::vector<UIElementData> &uiElements);

    LevelManagerConfig m_config;
    std::shared_ptr<GameScene> m_gameScene;
    std::string m_currentMapPath;
    std::vector<std::string> m_scenes;

    BoundingBox m_playerSpawnZone;
    Texture2D m_spawnTexture;
    bool m_hasSpawnZone = false;
    bool m_spawnTextureLoaded = false;

    std::unique_ptr<MapCollisionInitializer> m_collisionInitializer;

    // Dependencies
    ModelLoader *m_modelLoader;
    RenderManager *m_renderManager;
    std::shared_ptr<IPlayer> m_player;
    std::shared_ptr<IMenu> m_menu;
    std::shared_ptr<CHEngine::Scene> m_activeScene;
};

} // namespace CHEngine

#endif // LEVELMANAGER_H
