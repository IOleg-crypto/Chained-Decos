#ifndef GAME_H
#define GAME_H

#include "Engine/Collision/CollisionManager.h"
#include "Engine/Engine.h"
#include "Engine/Model/Model.h"
#include "Engine/World/World.h"
#include "Engine/Map/MapLoader.h"
#include "Engine/Kernel/Kernel.h"
#include "Player/Player.h"
#include <memory>

class Menu;
class MapManager;
class ResourceManager;
class StateManager;
class RenderHelper;

class Game
{
private:
    float PLAYER_SAFE_SPAWN_HEIGHT = 2.0f;
    
    // Core components
    std::unique_ptr<Player> m_player;
    std::unique_ptr<CollisionManager> m_collisionManager;
    std::unique_ptr<ModelLoader> m_models;
    std::unique_ptr<WorldManager> m_world;
    std::unique_ptr<Menu> m_menu;
    std::unique_ptr<Engine> m_engine;
    std::unique_ptr<Kernel> m_kernel;

    // Game manager components
    std::unique_ptr<MapManager> m_mapManager;
    std::unique_ptr<ResourceManager> m_modelManager;
    std::unique_ptr<StateManager> m_stateManager;
    std::unique_ptr<RenderHelper> m_renderHelper;

    bool m_showMenu;
    bool m_isGameInitialized;
    [[maybe_unused]] bool m_isDebugInfo;

public:
    Game();
    ~Game();

    void Init(int argc, char *argv[]);
    void Run();
    void Update();
    void Render();
    void Cleanup();

    void ToggleMenu();
    void RequestExit() const;
    bool IsRunning() const;

    void EnableCursor();
    void HideCursor();

    void InitInput();
    void InitPlayer();
    
    void UpdatePlayerLogic();
    void UpdatePhysicsLogic();

    void HandleMenuActions();
    void RenderGameWorld();
    void RenderGameUI() const;

    // Delegated methods to managers
    void InitCollisions();
    void InitCollisionsWithModels(const std::vector<std::string> &requiredModels);
    bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels);
    
    std::optional<ModelLoader::LoadResult> LoadGameModels();
    std::optional<ModelLoader::LoadResult> LoadGameModelsSelective(const std::vector<std::string> &modelNames);
    std::optional<ModelLoader::LoadResult> LoadGameModelsSelectiveSafe(const std::vector<std::string> &modelNames);
    std::string GetModelNameForObjectType(int objectType, const std::string &modelName = "");
    std::vector<std::string> GetModelsRequiredForMap(const std::string &mapIdentifier);

    void CreatePlatform(const Vector3 &position, const Vector3 &size, Color color, CollisionType collisionType);
    static float CalculateDynamicFontSize(float baseSize);

    void LoadEditorMap(const std::string &mapPath);
    void RenderEditorMap();
    void DumpMapDiagnostics() const;

    void SaveGameState();
    void RestoreGameState();

    Player &GetPlayer() { return *m_player; }
    CollisionManager &GetCollisionManager() { return *m_collisionManager; }
    ModelLoader &GetModels() { return *m_models; }
    WorldManager &GetWorld() { return *m_world; }
    Menu &GetMenu();
    Kernel &GetKernel() { return *m_kernel; }
    
    MapManager* GetMapManager() { return m_mapManager.get(); }
    ResourceManager* GetModelManager() { return m_modelManager.get(); }
    StateManager* GetStateManager() { return m_stateManager.get(); }
    RenderHelper* GetRenderHelper() { return m_renderHelper.get(); }
    
    GameMap &GetGameMap();
    bool IsInitialized() const { return m_isGameInitialized; }
};

#endif // GAME_H
