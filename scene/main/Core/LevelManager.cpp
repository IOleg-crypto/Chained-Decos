#include "LevelManager.h"
#include "core/engine/Engine.h"
#include "scene/main/Core/MapCollisionInitializer.h"
#include "scene/resources/map/Core/MapService.h"
#include "scene/resources/map/Renderer/MapRenderer.h"
#include <filesystem>
#include <fstream>
#include <set>

LevelManager::LevelManager(const LevelManagerConfig &config)
    : m_config(config), m_gameMap(std::make_unique<GameMap>()), m_currentMapPath(""),
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
    if (m_gameMap)
        m_gameMap->Cleanup();
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

bool LevelManager::Initialize(Engine *engine)
{
    if (!engine)
    {
        TraceLog(LOG_ERROR, "[LevelManager] Engine is null");
        return false;
    }

    m_engine = engine;

    // Get required dependencies from Engine (GetService returns shared_ptr)
    auto worldMgr = engine->GetService<WorldManager>();
    m_worldManager = worldMgr ? worldMgr.get() : nullptr;

    auto collMgr = engine->GetService<CollisionManager>();
    m_collisionManager = collMgr ? collMgr.get() : nullptr;

    auto modelLdr = engine->GetService<ModelLoader>();
    m_modelLoader = modelLdr ? modelLdr.get() : nullptr;

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
        std::make_unique<MapCollisionInitializer>(m_collisionManager, m_modelLoader);

    return true;
}

bool LevelManager::LoadMap(const std::string &path)
{
    LoadEditorMap(path);
    return IsMapLoaded();
}

void LevelManager::UnloadMap()
{
    if (m_gameMap)
        m_gameMap->Cleanup();
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
        auto player = m_engine->GetPlayer();
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

void LevelManager::RegisterServices(Engine *engine)
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
        m_collisionInitializer->InitializeCollisions(*m_gameMap);
    }
}

void LevelManager::InitCollisionsWithModels(const std::vector<std::string> &requiredModels)
{
    if (m_collisionInitializer)
    {
        m_collisionInitializer->InitializeCollisionsWithModels(*m_gameMap, requiredModels);
    }
}

bool LevelManager::InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels)
{
    if (m_collisionInitializer)
    {
        return m_collisionInitializer->InitializeCollisionsWithModelsSafe(*m_gameMap,
                                                                          requiredModels);
    }
    return false;
}

void LevelManager::SetPlayer(IPlayer *player)
{
    m_player = player;
    if (m_collisionInitializer)
    {
        m_collisionInitializer->SetPlayer(player);
    }
}

GameMap &LevelManager::GetGameMap()
{
    return *m_gameMap;
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

    for (const auto &object : m_gameMap->GetMapObjects())
    {
        if (object.type == MapObjectType::MODEL || object.type == MapObjectType::SPAWN_ZONE)
        {
            continue;
        }

        renderer.RenderMapObject(object, m_gameMap->GetMapModels(), dummyCamera, false);
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
            std::string(PROJECT_ROOT_DIR) + "/resources/boxes/PlayerSpawnTexture.png";
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

    for (const auto &pair : m_gameMap->GetMapModels())
    {
        m_modelLoader->UnloadModel(pair.first);
    }

    m_gameMap->Cleanup();
    m_hasSpawnZone = false;
    m_playerSpawnZone = {0};
    m_collisionManager->ClearColliders();

    std::string extension = std::filesystem::path(mapPath).extension().string();
    if (extension == ".json" || extension == ".JSON")
    {
        MapService mapService;
        if (!mapService.LoadMap(mapPath, *m_gameMap))
        {
            TraceLog(LOG_ERROR, "LevelManager::LoadEditorMap() - MapService failed to load map: %s",
                     mapPath.c_str());
            return;
        }

        m_currentMapPath = mapPath;

        if (!m_gameMap->GetMapModels().empty())
        {
            for (const auto &p : m_gameMap->GetMapModels())
            {
                if (p.second.meshCount > 0)
                {
                    m_modelLoader->RegisterLoadedModel(p.first, p.second);
                }
            }
        }

        if (!m_gameMap->GetMapMetaData().skyboxTexture.empty())
        {
            MapLoader mapLoader;
            mapLoader.LoadSkyboxForMap(*m_gameMap);
        }
    }

    for (size_t i = 0; i < m_gameMap->GetMapObjects().size(); ++i)
    {
        const auto &object = m_gameMap->GetMapObjects()[i];

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

    if (m_gameMap->GetMapMetaData().startPosition.x != 0.0f ||
        m_gameMap->GetMapMetaData().startPosition.y != 0.0f ||
        m_gameMap->GetMapMetaData().startPosition.z != 0.0f)
    {
        const float spawnSize = 2.0f;
        Vector3 spawnPos = m_gameMap->GetMapMetaData().startPosition;
        m_playerSpawnZone.min = {spawnPos.x - spawnSize / 2, spawnPos.y - spawnSize / 2,
                                 spawnPos.z - spawnSize / 2};
        m_playerSpawnZone.max = {spawnPos.x + spawnSize / 2, spawnPos.y + spawnSize / 2,
                                 spawnPos.z + spawnSize / 2};
        m_hasSpawnZone = true;
    }

    m_collisionManager->Initialize();

    // Create instances for all MODEL objects
    auto available = m_modelLoader->GetAvailableModels();
    for (const auto &object : m_gameMap->GetMapObjects())
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
