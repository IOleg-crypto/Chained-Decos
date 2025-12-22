#include "LevelManager.h"
#include "core/Engine.h"
#include "scene/main/core/MapCollisionInitializer.h"
#include "scene/resources/map/core/MapService.h"
#include "scene/resources/map/renderer/MapRenderer.h"
#include "scene/resources/model/utils/ModelAnalyzer.h"
#include <filesystem>

LevelManager::LevelManager(const LevelManagerConfig &config)
    : m_config(config), m_gameScene(std::make_unique<GameScene>()), m_currentMapPath(""),
      m_playerSpawnZone({0}), m_spawnTexture({0}), m_hasSpawnZone(false),
      m_spawnTextureLoaded(false), m_collisionInitializer(nullptr), m_worldManager(nullptr),
      m_collisionManager(nullptr), m_modelLoader(nullptr), m_renderManager(nullptr),
      m_player(nullptr), m_menu(nullptr), m_engine(nullptr)
{
}

LevelManager::~LevelManager()
{
    Shutdown();
}

void LevelManager::Shutdown()
{
    if (m_gameScene)
        m_gameScene->Cleanup();
    m_currentMapPath = "";
    m_hasSpawnZone = false;
    if (m_spawnTextureLoaded)
    {
        UnloadTexture(m_spawnTexture);
        m_spawnTextureLoaded = false;
    }
    m_collisionInitializer = nullptr;
    m_worldManager = nullptr;
    m_collisionManager = nullptr;
    m_modelLoader = nullptr;
    m_renderManager = nullptr;
    m_player = nullptr;
    m_menu = nullptr;
    m_engine = nullptr;
}

bool LevelManager::Initialize(ChainedEngine::Engine *engine)
{
    if (!engine)
    {
        TraceLog(LOG_ERROR, "[LevelManager] Engine is null");
        return false;
    }

    m_engine = engine;

    // Get required dependencies from Engine
    m_worldManager = std::static_pointer_cast<WorldManager>(engine->GetWorldManager());
    m_collisionManager = std::static_pointer_cast<CollisionManager>(engine->GetCollisionManager());
    m_modelLoader = std::static_pointer_cast<ModelLoader>(engine->GetModelLoader());

    // RenderManager is accessed directly through Engine
    m_renderManager = engine->GetRenderManager();

    // Validate required engine dependencies
    if (!m_worldManager || !m_collisionManager || !m_modelLoader || !m_renderManager)
    {
        TraceLog(LOG_ERROR, "[LevelManager] Required engine services not found");
        return false;
    }

    // Create collision initializer with dependencies
    m_collisionInitializer =
        std::make_unique<MapCollisionInitializer>(m_collisionManager, m_modelLoader, m_player);

    return true;
}

bool LevelManager::LoadScene(const std::string &path)
{
    std::string mapPath = path;

    // Convert map name to path if it's not already a path
    if (path.find('/') == std::string::npos && path.find('\\') == std::string::npos)
    {
        mapPath = ConvertMapNameToPath(path);
    }

    TraceLog(LOG_INFO, "[LevelManager] Loading level: %s", mapPath.c_str());

    try
    {
        // 1. Analyze map for required models
        std::vector<std::string> requiredModels = ModelAnalyzer::GetModelsRequiredForMap(mapPath);
        if (requiredModels.empty())
        {
            requiredModels.emplace_back("player_low"); // Always need player
        }

        // 2. Load required models selectively
        if (m_modelLoader)
        {
            m_modelLoader->LoadGameModelsSelective(requiredModels);
        }

        // 3. Initialize collision system with models
        if (!InitCollisionsWithModelsSafe(requiredModels))
        {
            TraceLog(LOG_ERROR, "[LevelManager] Failed to initialize collision system for map: %s",
                     mapPath.c_str());
            return false;
        }

        // 4. Load map objects and instances
        LoadEditorMap(mapPath);

        TraceLog(LOG_INFO, "[LevelManager] Level loaded successfully: %s", mapPath.c_str());
        return true;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "[LevelManager] Failed to load level %s: %s", mapPath.c_str(),
                 e.what());
        return false;
    }
}

std::string LevelManager::ConvertMapNameToPath(const std::string &mapName)
{
    if (mapName.empty())
        return "";

    // If it's already an absolute path or has extension, return as is
    if (std::filesystem::path(mapName).is_absolute() ||
        std::filesystem::path(mapName).has_extension())
    {
        return mapName;
    }

    // Otherwise, assume it's in resources/maps/ and needs .json
    std::string path = std::string(PROJECT_ROOT_DIR) + "/resources/maps/" + mapName + ".json";
    return path;
}

void LevelManager::UnloadMap()
{
    if (m_gameScene)
        m_gameScene->Cleanup();
    m_currentMapPath = "";
    m_hasSpawnZone = false;
}

bool LevelManager::IsMapLoaded() const
{
    return !m_currentMapPath.empty();
}

void LevelManager::Update(float deltaTime)
{
    // Update Player reference if it became available
    if (!m_player && m_engine && m_collisionInitializer)
    {
        auto player = m_engine->GetService<IPlayer>();
        if (player)
        {
            m_player = player;
            m_collisionInitializer->SetPlayer(m_player);
        }
    }

    // Map update logic if needed
    (void)deltaTime;
}

void LevelManager::Render()
{
    // Rendering is handled by RenderingSystem or MapRenderer
}

void LevelManager::RegisterServices(ChainedEngine::Engine *engine)
{
    if (!engine)
    {
        return;
    }

    // Register LevelManager directly
    engine->RegisterService<ILevelManager>(
        std::shared_ptr<ILevelManager>(this, [](ILevelManager *) {}));
}

std::vector<std::string> LevelManager::GetDependencies() const
{
    return {};
}

// Collision initialization methods
void LevelManager::InitCollisions()
{
    if (m_collisionInitializer)
    {
        m_collisionInitializer->InitializeCollisions(*m_gameScene);
    }
}

void LevelManager::InitCollisionsWithModels(const std::vector<std::string> &requiredModels)
{
    if (m_collisionInitializer)
    {
        m_collisionInitializer->InitializeCollisionsWithModels(*m_gameScene, requiredModels);
    }
}

bool LevelManager::InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels)
{
    if (m_collisionInitializer)
    {
        return m_collisionInitializer->InitializeCollisionsWithModelsSafe(*m_gameScene,
                                                                          requiredModels);
    }
    return false;
}

void LevelManager::SetPlayer(std::shared_ptr<IPlayer> player)
{
    m_player = std::move(player);
    if (m_collisionInitializer)
    {
        m_collisionInitializer->SetPlayer(m_player);
    }
}

GameScene &LevelManager::GetGameScene()
{
    return *m_gameScene;
}

const std::string &LevelManager::GetCurrentMapPath() const
{
    return m_currentMapPath;
}

std::string LevelManager::GetCurrentMapName() const
{
    return GetCurrentMapPath();
}

Vector3 LevelManager::GetSpawnPosition() const
{
    return GetPlayerSpawnPosition();
}

Vector3 LevelManager::GetPlayerSpawnPosition() const
{
    if (!m_hasSpawnZone)
    {
        return {0.0f, 0.0f, 0.0f};
    }

    return {(m_playerSpawnZone.min.x + m_playerSpawnZone.max.x) * 0.5f,
            (m_playerSpawnZone.min.y + m_playerSpawnZone.max.y) * 0.5f,
            (m_playerSpawnZone.min.z + m_playerSpawnZone.max.z) * 0.5f};
}

void LevelManager::RenderEditorMap()
{
    MapRenderer renderer;
    Camera3D dummyCamera = {0};

    for (const auto &object : m_gameScene->GetMapObjects())
    {
        if (object.type == MapObjectType::MODEL || object.type == MapObjectType::SPAWN_ZONE)
        {
            continue;
        }

        renderer.RenderMapObject(object, m_gameScene->GetMapModels(), dummyCamera, false);
    }
}

void LevelManager::RenderSpawnZone() const
{
    if (!m_hasSpawnZone)
    {
        return;
    }

    if (!m_spawnTextureLoaded)
    {
        std::string texturePath =
            std::string(PROJECT_ROOT_DIR) + "/resources/boxes/playerSpawnTexture.png";
        if (FileExists(texturePath.c_str()))
        {
            auto *self = const_cast<LevelManager *>(this);
            self->m_spawnTexture = LoadTexture(texturePath.c_str());
            if (self->m_spawnTexture.id != 0)
            {
                self->m_spawnTextureLoaded = true;
            }
        }
    }

    if (m_spawnTextureLoaded)
    {
        Vector3 size = {m_playerSpawnZone.max.x - m_playerSpawnZone.min.x,
                        m_playerSpawnZone.max.y - m_playerSpawnZone.min.y,
                        m_playerSpawnZone.max.z - m_playerSpawnZone.min.z};

        Vector3 center = {(m_playerSpawnZone.min.x + m_playerSpawnZone.max.x) * 0.5f,
                          (m_playerSpawnZone.min.y + m_playerSpawnZone.max.y) * 0.5f,
                          (m_playerSpawnZone.min.z + m_playerSpawnZone.max.z) * 0.5f};

        MapRenderer renderer;
        float spawnSize = (size.x + size.y + size.z) / 3.0f;
        renderer.RenderSpawnZone(m_spawnTexture, center, spawnSize, WHITE, true);
    }
}

void LevelManager::DumpMapDiagnostics() const
{
}

void LevelManager::LoadEditorMap(const std::string &mapPath)
{

    if (!std::filesystem::exists(mapPath))
    {
        TraceLog(LOG_ERROR, "LevelManager::LoadEditorMap() - Map file does not exist: %s",
                 mapPath.c_str());
        return;
    }

    m_modelLoader->ClearInstances();

    for (const auto &pair : m_gameScene->GetMapModels())
    {
        m_modelLoader->UnloadModel(pair.first);
    }

    m_gameScene->Cleanup();
    m_hasSpawnZone = false;
    m_playerSpawnZone = {0};
    m_collisionManager->ClearColliders();

    std::string extension = std::filesystem::path(mapPath).extension().string();
    if (extension == ".json" || extension == ".JSON")
    {
        MapService mapService;
        if (!mapService.LoadScene(mapPath, *m_gameScene))
        {
            TraceLog(LOG_ERROR, "LevelManager::LoadEditorMap() - MapService failed to load map: %s",
                     mapPath.c_str());
            return;
        }

        m_currentMapPath = mapPath;

        if (!m_gameScene->GetMapModels().empty())
        {
            for (const auto &p : m_gameScene->GetMapModels())
            {
                if (p.second.meshCount > 0)
                {
                    m_modelLoader->RegisterLoadedModel(p.first, p.second);
                }
            }
        }

        if (!m_gameScene->GetMapMetaData().skyboxTexture.empty())
        {
            SceneLoader mapLoader;
            mapLoader.LoadSkyboxForScene(*m_gameScene);
        }
    }

    for (size_t i = 0; i < m_gameScene->GetMapObjects().size(); ++i)
    {
        const auto &object = m_gameScene->GetMapObjects()[i];

        if (!std::isfinite(object.position.x) || !std::isfinite(object.position.y) ||
            !std::isfinite(object.position.z))
            continue;

        Vector3 colliderSize = object.scale;
        bool useBVHCollision = false;
        Collision collision;

        switch (object.type)
        {
        case MapObjectType::CUBE:
            colliderSize = Vector3{std::abs(object.scale.x != 0.0f ? object.scale.x : 1.0f),
                                   std::abs(object.scale.y != 0.0f ? object.scale.y : 1.0f),
                                   std::abs(object.scale.z != 0.0f ? object.scale.z : 1.0f)};
            break;
        case MapObjectType::MODEL:
        {
            const Model *modelPtr = nullptr;
            if (!object.modelName.empty())
            {
                auto modelOpt = m_modelLoader->GetModelByName(object.modelName);
                if (modelOpt)
                    modelPtr = &modelOpt->get();
            }

            if (modelPtr)
            {
                Matrix translation =
                    MatrixTranslate(object.position.x, object.position.y, object.position.z);
                Matrix scaleMatrix = MatrixScale(object.scale.x, object.scale.y, object.scale.z);
                Vector3 rot = {object.rotation.x * DEG2RAD, object.rotation.y * DEG2RAD,
                               object.rotation.z * DEG2RAD};
                Matrix rotationMatrix = MatrixRotateXYZ(rot);
                Matrix transform =
                    MatrixMultiply(scaleMatrix, MatrixMultiply(rotationMatrix, translation));

                collision = Collision();
                collision.BuildFromModelWithType(const_cast<Model *>(modelPtr),
                                                 CollisionType::BVH_ONLY, transform);
                useBVHCollision = true;
            }
        }
        break;
        default:
            break;
        }

        if (useBVHCollision)
        {
            collision.SetCollisionType(CollisionType::BVH_ONLY);
            m_collisionManager->AddCollider(std::make_shared<Collision>(std::move(collision)));
        }
        else if (object.type != MapObjectType::LIGHT && object.type != MapObjectType::SPAWN_ZONE)
        {
            Vector3 halfSize = Vector3Scale(colliderSize, 0.5f);
            collision = Collision(object.position, halfSize);
            collision.SetCollisionType(CollisionType::AABB_ONLY);
            m_collisionManager->AddCollider(std::make_shared<Collision>(std::move(collision)));
        }
    }

    if (m_gameScene->GetMapMetaData().startPosition.x != 0.0f ||
        m_gameScene->GetMapMetaData().startPosition.y != 0.0f ||
        m_gameScene->GetMapMetaData().startPosition.z != 0.0f)
    {
        const float spawnSize = 2.0f;
        Vector3 spawnPos = m_gameScene->GetMapMetaData().startPosition;
        m_playerSpawnZone.min = {spawnPos.x - spawnSize / 2, spawnPos.y - spawnSize / 2,
                                 spawnPos.z - spawnSize / 2};
        m_playerSpawnZone.max = {spawnPos.x + spawnSize / 2, spawnPos.y + spawnSize / 2,
                                 spawnPos.z + spawnSize / 2};
        m_hasSpawnZone = true;
    }

    m_collisionManager->Initialize();

    // Create instances for all MODEL objects
    auto available = m_modelLoader->GetAvailableModels();
    for (const auto &object : m_gameScene->GetMapObjects())
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
        {
            std::string candidateName = object.modelName;
            bool exists =
                (std::find(available.begin(), available.end(), candidateName) != available.end());

            if (!exists)
            {
                std::string stem = std::filesystem::path(candidateName).stem().string();
                if (std::find(available.begin(), available.end(), stem) != available.end())
                {
                    candidateName = stem;
                    exists = true;
                }
            }

            if (exists)
            {
                ModelInstanceConfig cfg;
                cfg.position = object.position;
                cfg.rotation = object.rotation;
                cfg.scale = (object.scale.x != 0.0f) ? object.scale.x : 1.0f;
                cfg.color = object.color;
                cfg.spawn = true;
                m_modelLoader->AddInstanceEx(candidateName, cfg);
            }
        }
    }
}
