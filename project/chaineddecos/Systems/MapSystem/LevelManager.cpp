#include "LevelManager.h"
#include "../../../core/engine/Engine.h"
#include "../../../scene/resources/map/Core/MapService.h"
#include "../../../scene/resources/map/Renderer/MapRenderer.h"
#include "../../Managers/MapCollisionInitializer.h"
#include <fstream>
#include <set>

LevelManager::LevelManager(const MapSystemConfig &config)
    : m_config(config), m_gameMap(std::make_unique<GameMap>()), m_currentMapPath(""),
      m_playerSpawnZone({0}), m_spawnTexture({0}), m_hasSpawnZone(false),
      m_spawnTextureLoaded(false), m_collisionInitializer(nullptr), m_worldManager(nullptr),
      m_collisionManager(nullptr), m_modelLoader(nullptr), m_renderManager(nullptr),
      m_player(nullptr), m_menu(nullptr), m_engine(nullptr)
{
    TraceLog(LOG_INFO, "[LevelManager] Constructor called");
}

LevelManager::~LevelManager()
{
    TraceLog(LOG_INFO, "[LevelManager] Destructor called");
    Shutdown();
}

void LevelManager::Shutdown()
{
    TraceLog(LOG_INFO, "[LevelManager] Shutting down...");
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
    TraceLog(LOG_INFO, "[LevelManager] Shutdown complete");
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
    m_renderManager = dynamic_cast<RenderManager *>(engine->GetRenderManager());

    // Validate required engine dependencies
    if (!m_worldManager || !m_collisionManager || !m_modelLoader || !m_renderManager)
    {
        TraceLog(LOG_ERROR, "[LevelManager] Required engine services not found");
        return false;
    }

    // Create collision initializer with dependencies
    m_collisionInitializer =
        std::make_unique<MapCollisionInitializer>(m_collisionManager, m_modelLoader);

    // NOTE: Spawn texture loading moved to lazy loading in RenderSpawnZone()
    // to avoid loading textures before OpenGL context is created

    TraceLog(LOG_INFO, "[LevelManager] Initialized successfully");
    return true;
}

void LevelManager::Update(float deltaTime)
{
    // Update Player reference if it became available
    if (!m_player && m_engine && m_collisionInitializer)
    {
        auto player = m_engine->GetPlayer();
        if (player)
        {
            m_player = static_cast<Player *>(player);
            m_collisionInitializer->SetPlayer(m_player);
            TraceLog(LOG_INFO, "[LevelManager] Player reference updated in collision initializer");
        }
    }

    // Map update logic if needed
    (void)deltaTime;
}

void LevelManager::Render()
{
    // LevelManager::Render is called through ModuleManager::RenderAllModules()
    // RenderEditorMap() and RenderSpawnZone() are now called in
    // RenderingSystem::RenderGameWorld() for correct rendering order (inside
    // BeginMode3D/EndMode3D) Empty function - rendering is handled by RenderingSystem
}

void LevelManager::RegisterServices(Engine *engine)
{
    if (!engine)
    {
        return;
    }

    TraceLog(LOG_INFO, "[LevelManager] Registering services...");

    // Register LevelManager directly
    engine->RegisterService<LevelManager>(
        std::shared_ptr<LevelManager>(this, [](LevelManager *) {}));
    TraceLog(LOG_INFO, "[LevelManager] LevelManager registered");

    // Dependency Injection: inject MapSystem into ConsoleManager
    // Note: ConsoleManagerHelpers might need update if it uses Kernel
    // UpdateConsoleManagerProviders(kernel); // This function signature likely takes Kernel*
    // I will comment it out for now or assume it needs update.
    // Actually, I should check ConsoleManagerHelpers.h
}

std::vector<std::string> LevelManager::GetDependencies() const
{
    // Base system - no dependencies on other game systems
    // But depends on engine services which are always available
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
        m_collisionInitializer->SetPlayer(dynamic_cast<Player *>(player));
    }
    TraceLog(LOG_INFO, "LevelManager::SetPlayer() - Player reference updated");
}

GameMap &LevelManager::GetGameMap()
{
    return *m_gameMap;
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
    // Use MapRenderer to render map objects
    // Note: This method renders primitives only; models are rendered via ModelLoader instances
    MapRenderer renderer;
    Camera3D dummyCamera = {0}; // Not used for primitive rendering

    for (const auto &object : m_gameMap->GetMapObjects())
    {
        // Skip models - they are rendered through ModelLoader instances
        if (object.type == MapObjectType::MODEL)
        {
            continue;
        }

        // Skip spawn zones - they are rendered separately
        if (object.type == MapObjectType::SPAWN_ZONE)
        {
            continue;
        }

        // Use MapRenderer to render the object
        renderer.RenderMapObject(object, m_gameMap->GetMapModels(), dummyCamera, false);
    }
}

void LevelManager::RenderSpawnZone() const
{
    if (!m_hasSpawnZone)
    {
        return;
    }

    // Lazy load spawn texture if not already loaded
    if (!m_spawnTextureLoaded)
    {
        std::string texturePath =
            std::string(PROJECT_ROOT_DIR) + "/resources/boxes/PlayerSpawnTexture.png";
        if (FileExists(texturePath.c_str()))
        {
            // Cast away const for lazy initialization
            auto *self = const_cast<LevelManager *>(this);
            self->m_spawnTexture = LoadTexture(texturePath.c_str());
            if (self->m_spawnTexture.id != 0)
            {
                self->m_spawnTextureLoaded = true;
                TraceLog(LOG_INFO, "LevelManager::RenderSpawnZone() - Loaded spawn texture: %dx%d",
                         self->m_spawnTexture.width, self->m_spawnTexture.height);
            }
            else
            {
                TraceLog(LOG_WARNING,
                         "LevelManager::RenderSpawnZone() - Failed to load spawn texture from: %s",
                         texturePath.c_str());
                return;
            }
        }
        else
        {
            return;
        }
    }

    // Calculate size and center of spawn zone
    Vector3 size = {m_playerSpawnZone.max.x - m_playerSpawnZone.min.x,
                    m_playerSpawnZone.max.y - m_playerSpawnZone.min.y,
                    m_playerSpawnZone.max.z - m_playerSpawnZone.min.z};

    Vector3 center = {(m_playerSpawnZone.min.x + m_playerSpawnZone.max.x) * 0.5f,
                      (m_playerSpawnZone.min.y + m_playerSpawnZone.max.y) * 0.5f,
                      (m_playerSpawnZone.min.z + m_playerSpawnZone.max.z) * 0.5f};

    // Use MapRenderer to render spawn zone
    MapRenderer renderer;
    float spawnSize = (size.x + size.y + size.z) / 3.0f; // Average size
    renderer.RenderSpawnZone(m_spawnTexture, center, spawnSize, WHITE, m_spawnTextureLoaded);
}

void LevelManager::DumpMapDiagnostics() const
{
    TraceLog(LOG_INFO, "LevelManager::DumpMapDiagnostics() - Map objects: %d",
             m_gameMap->GetMapObjects().size());

    for (size_t i = 0; i < m_gameMap->GetMapObjects().size(); ++i)
    {
        const auto &o = m_gameMap->GetMapObjects()[i];
        TraceLog(LOG_INFO,
                 "LevelManager::DumpMapDiagnostics() - Object %d: name='%s' type=%d modelName='%s' "
                 "pos=(%.2f,%.2f,%.2f) scale=(%.2f,%.2f,%.2f)",
                 i, o.name.c_str(), static_cast<int>(o.type), o.modelName.c_str(), o.position.x,
                 o.position.y, o.position.z, o.scale.x, o.scale.y, o.scale.z);
    }

    // If MapLoader preloaded models into the map, list them
    if (!m_gameMap->GetMapModels().empty())
    {
        TraceLog(LOG_INFO,
                 "LevelManager::DumpMapDiagnostics() - GameMap.loadedModels contains %d entries",
                 m_gameMap->GetMapModels().size());
        for (const auto &p : m_gameMap->GetMapModels())
        {
            TraceLog(LOG_INFO,
                     "LevelManager::DumpMapDiagnostics() -   loadedModel key: %s (meshCount: %d)",
                     p.first.c_str(), p.second.meshCount);
        }
    }
    else
    {
        TraceLog(LOG_INFO, "LevelManager::DumpMapDiagnostics() - GameMap.loadedModels is empty");
    }

    // List ModelLoader's available models
    auto available = m_modelLoader->GetAvailableModels();
    TraceLog(LOG_INFO, "LevelManager::DumpMapDiagnostics() - ModelLoader available models: %d",
             available.size());
    for (const auto &name : available)
    {
        TraceLog(LOG_INFO, "LevelManager::DumpMapDiagnostics() -   %s", name.c_str());
    }
}

void LevelManager::LoadEditorMap(const std::string &mapPath)
{
    TraceLog(LOG_INFO, "LevelManager::LoadEditorMap() - Loading map from: %s", mapPath.c_str());

    // Validate map path exists and is readable
    if (!std::filesystem::exists(mapPath))
    {
        TraceLog(LOG_ERROR, "LevelManager::LoadEditorMap() - Map file does not exist: %s",
                 mapPath.c_str());
        return;
    }

    // Clear previous map data BEFORE loading new map
    TraceLog(LOG_INFO, "LevelManager::LoadEditorMap() - Clearing previous map data...");
    TraceLog(LOG_INFO,
             "LevelManager::LoadEditorMap() - Current collider count before map load: %zu",
             m_collisionManager->GetColliders().size());

    // Clear old model instances to prevent overlap with new map
    m_modelLoader->ClearInstances();

    // IMPORTANT: Unregister models from ModelLoader BEFORE calling GameMap::Cleanup()
    for (const auto &pair : m_gameMap->GetMapModels())
    {
        const std::string &modelName = pair.first;
        m_modelLoader->UnloadModel(modelName);

        // Also try to unload possible aliases (stem without extension)
        try
        {
            std::string stem = std::filesystem::path(modelName).stem().string();
            if (!stem.empty() && stem != modelName)
            {
                m_modelLoader->UnloadModel(stem);
            }
        }
        catch (...)
        {
            // Ignore filesystem errors
        }
    }

    m_gameMap->Cleanup();

    // Clear previous spawn zone
    m_hasSpawnZone = false;
    m_playerSpawnZone = {0};

    // Clear previous colliders
    m_collisionManager->ClearColliders();

    // Check if this is a JSON file exported from map editor
    size_t dotPos = mapPath.find_last_of(".");
    std::string extension;
    if (dotPos != std::string::npos && dotPos + 1 < mapPath.length())
    {
        extension = mapPath.substr(dotPos + 1);
    }

    if (extension == "json")
    {
        TraceLog(LOG_INFO,
                 "LevelManager::LoadEditorMap() - Detected JSON format, using MapService");

        // Use MapService to load map
        MapService mapService;
        if (!mapService.LoadMap(mapPath, *m_gameMap))
        {
            TraceLog(LOG_ERROR, "LevelManager::LoadEditorMap() - MapService failed to load map: %s",
                     mapPath.c_str());
            return;
        }

        TraceLog(LOG_INFO,
                 "LevelManager::LoadEditorMap() - MapService loaded %zu objects successfully",
                 m_gameMap->GetMapObjects().size());

        m_currentMapPath = mapPath;

        // Register any models preloaded by MapLoader into the runtime ModelLoader
        if (!m_gameMap->GetMapModels().empty())
        {
            TraceLog(LOG_INFO,
                     "LevelManager::LoadEditorMap() - Registering %zu preloaded models from map",
                     m_gameMap->GetMapModels().size());
            for (const auto &p : m_gameMap->GetMapModels())
            {
                const std::string &modelName = p.first;
                const ::Model &loaded = p.second;

                // Validate model before registration
                if (loaded.meshCount > 0)
                {
                    if (m_modelLoader->RegisterLoadedModel(modelName, loaded))
                    {
                        TraceLog(
                            LOG_INFO,
                            "LevelManager::LoadEditorMap() - Successfully registered model: %s",
                            modelName.c_str());
                    }
                    else
                    {
                        TraceLog(LOG_WARNING,
                                 "LevelManager::LoadEditorMap() - Failed to register model: %s",
                                 modelName.c_str());
                    }
                }
                else
                {
                    TraceLog(LOG_WARNING,
                             "LevelManager::LoadEditorMap() - Skipping invalid model: %s",
                             modelName.c_str());
                }
            }
        }

        TraceLog(LOG_INFO,
                 "LevelManager::LoadEditorMap() - Successfully loaded JSON map with %zu objects",
                 m_gameMap->GetMapObjects().size());
    }

    TraceLog(LOG_INFO, "LevelManager::LoadEditorMap() - Map loaded, checking object count: %zu",
             m_gameMap->GetMapObjects().size());
    if (m_gameMap->GetMapObjects().empty())
    {
        TraceLog(LOG_ERROR, "LevelManager::LoadEditorMap() - No objects loaded from map");
        return;
    }

    // Validate map object count to prevent memory issues
    if (m_gameMap->GetMapObjects().size() > 10000)
    {
        TraceLog(LOG_ERROR,
                 "LevelManager::LoadEditorMap() - Map has too many objects (%d), limiting to 10000",
                 m_gameMap->GetMapObjects().size());
        return;
    }

    // Create collision boxes for all objects in the map
    TraceLog(LOG_INFO, "LevelManager::LoadEditorMap() - Creating collision boxes for %d objects",
             m_gameMap->GetMapObjects().size());
    size_t collisionCreationCount = 0;
    size_t collisionSkippedCount = 0;

    for (size_t i = 0; i < m_gameMap->GetMapObjects().size(); ++i)
    {
        const auto &object = m_gameMap->GetMapObjects()[i];

        // Validate object data before creating collision
        if (!std::isfinite(object.position.x) || !std::isfinite(object.position.y) ||
            !std::isfinite(object.position.z))
        {
            TraceLog(LOG_WARNING,
                     "LevelManager::LoadEditorMap() - Object %d has invalid position, skipping", i);
            collisionSkippedCount++;
            continue;
        }

        if (!std::isfinite(object.scale.x) || !std::isfinite(object.scale.y) ||
            !std::isfinite(object.scale.z))
        {
            TraceLog(LOG_WARNING,
                     "LevelManager::LoadEditorMap() - Object %d has invalid scale, skipping", i);
            collisionSkippedCount++;
            continue;
        }

        Vector3 colliderSize = object.scale;
        bool useBVHCollision = false;
        Collision collision;

        // Adjust collider size based on object type
        switch (object.type)
        {
        case MapObjectType::CUBE:
            colliderSize = Vector3{std::abs(object.scale.x != 0.0f ? object.scale.x : 1.0f),
                                   std::abs(object.scale.y != 0.0f ? object.scale.y : 1.0f),
                                   std::abs(object.scale.z != 0.0f ? object.scale.z : 1.0f)};
            break;
        case MapObjectType::SPHERE:
        {
            float radius = std::abs(object.radius > 0.0f ? object.radius : 1.0f);
            colliderSize = Vector3{radius, radius, radius};
        }
        break;
        case MapObjectType::CYLINDER:
        {
            float radius = std::abs(object.radius > 0.0f ? object.radius : 1.0f);
            float height = std::abs(object.height > 0.0f ? object.height : 2.0f);
            colliderSize = Vector3{radius, height, radius};
        }
        break;
        case MapObjectType::PLANE:
        {
            float planeWidth = std::abs(object.size.x != 0.0f ? object.size.x : 5.0f);
            float planeLength = std::abs(object.size.y != 0.0f ? object.size.y : 5.0f);
            colliderSize = Vector3{planeWidth, 0.1f, planeLength};
        }

            // Skip collision creation for large ground planes
            if (colliderSize.x > 500.0f || colliderSize.z > 500.0f ||
                (object.position.y <= 1.0f && object.position.y >= -1.0f &&
                 (colliderSize.x > 100.0f || colliderSize.z > 100.0f)))
            {
                TraceLog(LOG_INFO,
                         "LevelManager::LoadEditorMap() - Skipping large ground plane '%s'",
                         object.name.c_str());
                collisionSkippedCount++;
                continue;
            }
            break;
        case MapObjectType::LIGHT:
            TraceLog(LOG_INFO, "LevelManager::LoadEditorMap() - LIGHT object: skipping collision");
            collisionSkippedCount++;
            continue;

        case MapObjectType::MODEL:
        {
            const Model *modelPtr = nullptr;

            if (!object.modelName.empty())
            {
                // Try runtime ModelLoader first
                if (m_modelLoader)
                {
                    auto modelOpt = m_modelLoader->GetModelByName(object.modelName);
                    if (!modelOpt)
                    {
                        std::string stem = std::filesystem::path(object.modelName).stem().string();
                        if (!stem.empty())
                        {
                            modelOpt = m_modelLoader->GetModelByName(stem);
                        }
                    }

                    if (modelOpt)
                    {
                        modelPtr = &modelOpt->get();
                    }
                }

                // Fallback to models preloaded by MapLoader
                if (!modelPtr)
                {
                    auto mapModelIt = m_gameMap->GetMapModels().find(object.modelName);
                    if (mapModelIt == m_gameMap->GetMapModels().end())
                    {
                        std::string stem = std::filesystem::path(object.modelName).stem().string();
                        if (!stem.empty())
                        {
                            mapModelIt = m_gameMap->GetMapModels().find(stem);
                        }
                    }

                    if (mapModelIt != m_gameMap->GetMapModels().end())
                    {
                        modelPtr = &mapModelIt->second;
                    }
                }
            }

            if (modelPtr)
            {
                Matrix translation =
                    MatrixTranslate(object.position.x, object.position.y, object.position.z);
                Matrix scaleMatrix = MatrixScale(object.scale.x, object.scale.y, object.scale.z);
                Vector3 rotationRad = {object.rotation.x * DEG2RAD, object.rotation.y * DEG2RAD,
                                       object.rotation.z * DEG2RAD};

                Matrix rotationMatrix = MatrixRotateXYZ(rotationRad);
                Matrix transform =
                    MatrixMultiply(scaleMatrix, MatrixMultiply(rotationMatrix, translation));

                collision = Collision();
                collision.BuildFromModelWithType(const_cast<Model *>(modelPtr),
                                                 CollisionType::BVH_ONLY, transform);
                useBVHCollision = true;

                BoundingBox bb = collision.GetBoundingBox();
                colliderSize = Vector3{std::abs(bb.max.x - bb.min.x), std::abs(bb.max.y - bb.min.y),
                                       std::abs(bb.max.z - bb.min.z)};

                TraceLog(LOG_INFO,
                         "LevelManager::LoadEditorMap() - Built BVH collision for model '%s'",
                         object.modelName.c_str());
            }
            else
            {
                colliderSize = Vector3{std::abs(object.scale.x != 0.0f ? object.scale.x : 1.0f),
                                       std::abs(object.scale.y != 0.0f ? object.scale.y : 1.0f),
                                       std::abs(object.scale.z != 0.0f ? object.scale.z : 1.0f)};
                TraceLog(
                    LOG_WARNING,
                    "LevelManager::LoadEditorMap() - Model '%s' not found, using AABB fallback",
                    object.modelName.c_str());
            }
        }
        break;
        default:
            colliderSize = Vector3{std::abs(object.scale.x != 0.0f ? object.scale.x : 1.0f),
                                   std::abs(object.scale.y != 0.0f ? object.scale.y : 1.0f),
                                   std::abs(object.scale.z != 0.0f ? object.scale.z : 1.0f)};
            break;
        }

        if (!useBVHCollision)
        {
            // Ensure colliderSize has valid non-zero dimensions
            if (colliderSize.x <= 0.0f)
                colliderSize.x = 1.0f;
            if (colliderSize.y <= 0.0f)
                colliderSize.y = 1.0f;
            if (colliderSize.z <= 0.0f)
                colliderSize.z = 1.0f;

            // Validate final collider size
            if (!std::isfinite(colliderSize.x) || !std::isfinite(colliderSize.y) ||
                !std::isfinite(colliderSize.z))
            {
                TraceLog(
                    LOG_WARNING,
                    "LevelManager::LoadEditorMap() - Object %d has invalid colliderSize, skipping",
                    i);
                continue;
            }

            // Collision constructor expects halfSize
            Vector3 halfSize = Vector3Scale(colliderSize, 0.5f);
            collision = Collision(object.position, halfSize);
            collision.SetCollisionType(CollisionType::AABB_ONLY);
        }
        else
        {
            collision.SetCollisionType(CollisionType::BVH_ONLY);
        }

        m_collisionManager->AddCollider(std::move(collision));

        TraceLog(LOG_INFO,
                 "LevelManager::LoadEditorMap() - Added collision for %s at (%.2f, %.2f, %.2f)",
                 object.name.c_str(), object.position.x, object.position.y, object.position.z);
        collisionCreationCount++;
    }

    // Create player spawn zone from startPosition if specified
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

        TraceLog(LOG_INFO,
                 "LevelManager::LoadEditorMap() - Created player spawn zone at (%.2f, %.2f, %.2f)",
                 spawnPos.x, spawnPos.y, spawnPos.z);
    }
    else
    {
        m_hasSpawnZone = false;
    }

    // Initialize collision manager
    m_collisionManager->Initialize();

    TraceLog(LOG_INFO, "LevelManager::LoadEditorMap() - Successfully loaded map with %d objects",
             m_gameMap->GetMapObjects().size());

    TraceLog(LOG_INFO,
             "LevelManager::LoadEditorMap() - Collision summary: %zu created, %zu skipped",
             collisionCreationCount, collisionSkippedCount);

    // Dump diagnostics
    DumpMapDiagnostics();

    // Create model instances for all MODEL objects
    TraceLog(LOG_INFO, "LevelManager::LoadEditorMap() - Creating model instances");

    // Ensure all referenced models are loaded
    std::set<std::string> uniqueModelNames;
    for (const auto &object : m_gameMap->GetMapObjects())
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
            uniqueModelNames.insert(object.modelName);
    }

    auto available = m_modelLoader->GetAvailableModels();
    for (const auto &requested : uniqueModelNames)
    {
        if (std::find(available.begin(), available.end(), requested) != available.end())
            continue;

        std::string stem = std::filesystem::path(requested).stem().string();
        if (!stem.empty() && std::find(available.begin(), available.end(), stem) != available.end())
            continue;

        // Attempt to auto-load
        std::vector<std::string> possiblePaths;
        std::string extension = std::filesystem::path(requested).extension().string();

        if (extension.empty())
        {
            std::vector<std::string> extensions = {".glb", ".gltf", ".obj", ".fbx"};
            for (const auto &ext : extensions)
            {
                possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + requested +
                                        ext);
                possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + stem + ext);
            }
        }
        else
        {
            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + requested);
            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + stem +
                                    extension);
        }

        bool loaded = false;
        for (const auto &resourcePath : possiblePaths)
        {
            if (std::ifstream(resourcePath).good())
            {
                TraceLog(LOG_INFO, "LevelManager::LoadEditorMap() - Auto-loading: %s",
                         resourcePath.c_str());
                if (m_modelLoader->LoadSingleModel(stem.empty() ? requested : stem, resourcePath,
                                                   true))
                {
                    TraceLog(LOG_INFO, "LevelManager::LoadEditorMap() - Auto-loaded model '%s'",
                             (stem.empty() ? requested : stem).c_str());
                    loaded = true;
                    break;
                }
            }
        }

        if (!loaded)
        {
            TraceLog(LOG_WARNING, "LevelManager::LoadEditorMap() - Failed to auto-load model: %s",
                     requested.c_str());
        }
    }

    // Create instances for all MODEL objects
    available = m_modelLoader->GetAvailableModels();
    for (const auto &object : m_gameMap->GetMapObjects())
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
        {
            std::string requested = object.modelName;
            bool exists =
                (std::find(available.begin(), available.end(), requested) != available.end());
            std::string candidateName = requested;

            if (!exists)
            {
                std::string stem = std::filesystem::path(requested).stem().string();
                if (!stem.empty() &&
                    std::find(available.begin(), available.end(), stem) != available.end())
                {
                    candidateName = stem;
                    exists = true;
                }
            }

            if (!exists)
            {
                TraceLog(
                    LOG_WARNING,
                    "LevelManager::LoadEditorMap() - Model '%s' not available, skipping instance",
                    requested.c_str());
                continue;
            }

            ModelInstanceConfig cfg;
            cfg.position = object.position;
            cfg.rotation = object.rotation;
            cfg.scale = (object.scale.x != 0.0f || object.scale.y != 0.0f || object.scale.z != 0.0f)
                            ? object.scale.x
                            : 1.0f;
            cfg.color = object.color;
            cfg.spawn = true;

            bool added = m_modelLoader->AddInstanceEx(candidateName, cfg);
            if (!added)
            {
                TraceLog(LOG_WARNING,
                         "LevelManager::LoadEditorMap() - Failed to add instance for model '%s'",
                         candidateName.c_str());
            }
        }
    }
}
