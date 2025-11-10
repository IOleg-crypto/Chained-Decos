#include "MapManager.h"
#include "../Player/Player.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Render/RenderManager.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "Engine/Map/MapService.h"
#include "Engine/Map/MapObjectConverter.h"
#include "Game/Menu/Menu.h"
#include "Player/PlayerCollision.h"
#include <raylib.h>
#include <rlgl.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <unordered_set>
#include "Engine/Render/RenderUtils.h"

MapManager::MapManager(Player* player, CollisionManager* collisionManager, ModelLoader* models, RenderManager* renderManager, Kernel* kernel, Menu* menu)
    : m_player(player), m_collisionManager(collisionManager), m_models(models), m_renderManager(renderManager), m_kernel(kernel), m_menu(menu),
      m_hasSpawnZone(false), m_spawnTextureLoaded(false)
{
    // Initialize spawn zone
    m_playerSpawnZone = {0};
    m_spawnTexture = {0};
    
    // Load spawn texture
    std::string texturePath = std::string(PROJECT_ROOT_DIR) + "/resources/boxes/PlayerSpawnTexture.png";
    if (FileExists(texturePath.c_str()))
    {
        m_spawnTexture = LoadTexture(texturePath.c_str());
        if (m_spawnTexture.id != 0)
        {
            m_spawnTextureLoaded = true;
            TraceLog(LOG_INFO, "MapManager::MapManager() - Loaded spawn texture: %dx%d", 
                     m_spawnTexture.width, m_spawnTexture.height);
        }
        else
        {
            TraceLog(LOG_WARNING, "MapManager::MapManager() - Failed to load spawn texture from: %s", texturePath.c_str());
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "MapManager::MapManager() - Spawn texture not found at: %s", texturePath.c_str());
    }
    
    TraceLog(LOG_INFO, "MapManager created");
}

MapManager::~MapManager()
{
    // Unload spawn texture if loaded
    if (m_spawnTextureLoaded && m_spawnTexture.id != 0)
    {
        UnloadTexture(m_spawnTexture);
        TraceLog(LOG_INFO, "MapManager::~MapManager() - Unloaded spawn texture");
    }
}

void MapManager::LoadEditorMap(const std::string &mapPath)
{
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Loading map from: %s", mapPath.c_str());
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Debug: Map path logged for verification");

    // Validate map path exists and is readable
    if (!std::filesystem::exists(mapPath))
    {
        TraceLog(LOG_ERROR, "MapManager::LoadEditorMap() - Map file does not exist: %s", mapPath.c_str());
        return;
    }

    // Clear previous map data BEFORE loading new map
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Clearing previous map data...");
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Current collider count before map load: %zu",
             m_collisionManager->GetColliders().size());
    
    // Clear old model instances to prevent overlap with new map
    m_models->ClearInstances();
    
   
    
    // IMPORTANT: Unregister models from ModelLoader BEFORE calling GameMap::Cleanup()
    // to prevent double free - models registered from GameMap::loadedModels share
    // the same GPU resources, so we must unregister them first
    // Also try to unload aliases (stem, lowercase variants) that might have been registered
    for (const auto &pair : m_gameMap.GetMapModels())
    {
        const std::string &modelName = pair.first;
        // Try to unload from ModelLoader if it was registered
        // This prevents double free when GameMap::Cleanup() calls UnloadModel()
        m_models->UnloadModel(modelName);
        
        // Also try to unload possible aliases (stem without extension)
        try {
            std::string stem = std::filesystem::path(modelName).stem().string();
            if (!stem.empty() && stem != modelName) {
                m_models->UnloadModel(stem);
            }
        } catch (...) {
            // Ignore filesystem errors
        }
    }
    
    m_gameMap.Cleanup(); // This clears loadedModels and calls UnloadModel() - but models are already unregistered
    
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
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - File extension: %s", extension.c_str());

    if (extension == "json")
    {
        TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Detected JSON format, using MapService");

        // Use MapService to load map (unified service for Editor and Game)
        MapService mapService;
        if (!mapService.LoadMap(mapPath, m_gameMap))
        {
            TraceLog(LOG_ERROR, "MapManager::LoadEditorMap() - MapService failed to load map: %s",
                     mapPath.c_str());
            TraceLog(LOG_ERROR, "MapManager::LoadEditorMap() - This may indicate JSON parsing errors or "
                                "invalid map format");
            return;
        }

        TraceLog(LOG_INFO,
                 "MapManager::LoadEditorMap() - MapService loaded %zu objects successfully",
                 m_gameMap.GetMapObjects().size());

        m_currentMapPath = mapPath;

        // Register any models preloaded by MapLoader into the runtime ModelLoader
        if (!m_gameMap.GetMapModels().empty())
        {
            TraceLog(LOG_INFO,
                     "MapManager::LoadEditorMap() - Registering %zu preloaded models from map into "
                     "ModelLoader",
                     m_gameMap.GetMapModels().size());
            for (const auto &p : m_gameMap.GetMapModels())
            {
                const std::string &modelName = p.first;
                const ::Model &loaded = p.second;

                // Validate model before registration
                if (loaded.meshCount > 0)
                {
                    if (m_models->RegisterLoadedModel(modelName, loaded))
                    {
                        TraceLog(LOG_INFO,
                                 "MapManager::LoadEditorMap() - Successfully registered model from "
                                 "map: %s (meshCount: %d)",
                                 modelName.c_str(), loaded.meshCount);
                    }
                    else
                    {
                        TraceLog(
                            LOG_WARNING,
                            "MapManager::LoadEditorMap() - Failed to register model from map: %s",
                            modelName.c_str());
                    }
                }
                else
                {
                    TraceLog(LOG_WARNING,
                             "MapManager::LoadEditorMap() - Skipping invalid model from map: %s "
                             "(meshCount: %d)",
                             modelName.c_str(), loaded.meshCount);
                }
            }
        }
        else
        {
            TraceLog(LOG_INFO,
                     "MapManager::LoadEditorMap() - No preloaded models in GameMap to register");
        }

        TraceLog(LOG_INFO,
                 "MapManager::LoadEditorMap() - MapService import successful, processing %zu objects",
                 m_gameMap.GetMapObjects().size());

        TraceLog(LOG_INFO,
                 "MapManager::LoadEditorMap() - Successfully loaded JSON map with %zu objects",
                 m_gameMap.GetMapObjects().size());
    }

    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Map loaded, checking object count: %zu",
             m_gameMap.GetMapObjects().size());
    if (m_gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_ERROR, "MapManager::LoadEditorMap() - No objects loaded from map");
        return;
    }

    // Validate map object count to prevent memory issues
    if (m_gameMap.GetMapObjects().size() > 10000)
    {
        TraceLog(LOG_ERROR,
                 "MapManager::LoadEditorMap() - Map has too many objects (%d), limiting to 10000",
                 m_gameMap.GetMapObjects().size());
        return;
    }

    // Create collision boxes for all objects in the map
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Creating collision boxes for %d objects",
             m_gameMap.GetMapObjects().size());
    size_t collisionCreationCount = 0;
    size_t collisionSkippedCount = 0;

    for (size_t i = 0; i < m_gameMap.GetMapObjects().size(); ++i)
    {
        const auto &object = m_gameMap.GetMapObjects()[i];

        // Debug log for each object's details
        TraceLog(LOG_INFO,
                 "MapManager::LoadEditorMap() - Object %d details: name='%s', type=%d, modelName='%s', "
                 "position=(%.2f,%.2f,%.2f), scale=(%.2f,%.2f,%.2f), color=(%d,%d,%d,%d)",
                 i, object.name.c_str(), static_cast<int>(object.type), object.modelName.c_str(),
                 object.position.x, object.position.y, object.position.z, object.scale.x,
                 object.scale.y, object.scale.z, object.color.r, object.color.g, object.color.b,
                 object.color.a);

        // Validate object data before creating collision
        if (!std::isfinite(object.position.x) || !std::isfinite(object.position.y) ||
            !std::isfinite(object.position.z))
        {
            TraceLog(LOG_WARNING,
                     "MapManager::LoadEditorMap() - Object %d has invalid position, skipping collision",
                     i);
            collisionSkippedCount++;
            continue;
        }

        if (!std::isfinite(object.scale.x) || !std::isfinite(object.scale.y) ||
            !std::isfinite(object.scale.z))
        {
            TraceLog(LOG_WARNING,
                     "MapManager::LoadEditorMap() - Object %d has invalid scale, skipping collision", i);
            collisionSkippedCount++;
            continue;
        }

        TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Creating collision for object %d: %s", i,
                 object.name.c_str());
        TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Object %d position: (%.2f, %.2f, %.2f)", i,
                 object.position.x, object.position.y, object.position.z);

        Vector3 colliderSize = object.scale;
        bool useBVHCollision = false;
        Collision collision; // will be initialized per-type

        // Adjust collider size based on object type
        switch (object.type)
        {
        case MapObjectType::CUBE:
            // For cubes, use absolute values of scale to avoid negative sizes
            {
                colliderSize = Vector3{
                    std::abs(object.scale.x != 0.0f ? object.scale.x : 1.0f),
                    std::abs(object.scale.y != 0.0f ? object.scale.y : 1.0f),
                    std::abs(object.scale.z != 0.0f ? object.scale.z : 1.0f)
                };
            }
            TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Cube collision: size=(%.2f, %.2f, %.2f)",
                     colliderSize.x, colliderSize.y, colliderSize.z);
            break;
        case MapObjectType::SPHERE:
            // For spheres, use radius for all dimensions
            {
                float radius = std::abs(object.radius > 0.0f ? object.radius : 1.0f);
                colliderSize = Vector3{radius, radius, radius};
            }
            TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Sphere collision: size=(%.2f, %.2f, %.2f)",
                     colliderSize.x, colliderSize.y, colliderSize.z);
            break;
        case MapObjectType::CYLINDER:
            // For cylinders, use radius for x/z and height for y
            {
                float radius = std::abs(object.radius > 0.0f ? object.radius : 1.0f);
                float height = std::abs(object.height > 0.0f ? object.height : 2.0f);
                colliderSize = Vector3{radius, height, radius};
            }
            TraceLog(LOG_INFO,
                     "MapManager::LoadEditorMap() - Cylinder collision: size=(%.2f, %.2f, %.2f)",
                     colliderSize.x, colliderSize.y, colliderSize.z);
            break;
        case MapObjectType::PLANE:
            // For planes, use absolute values of size for x/z and small height for y
            {
                float planeWidth = std::abs(object.size.x != 0.0f ? object.size.x : 5.0f);
                float planeLength = std::abs(object.size.y != 0.0f ? object.size.y : 5.0f);
                colliderSize = Vector3{planeWidth, 0.1f, planeLength};
            }
            
            // Skip collision creation for large ground planes (artificial ground)
            // Large planes with y position near 0 are likely artificial ground
            if (colliderSize.x > 500.0f || colliderSize.z > 500.0f || 
                (object.position.y <= 1.0f && object.position.y >= -1.0f && 
                 (colliderSize.x > 100.0f || colliderSize.z > 100.0f)))
            {
                TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - PLANE object '%s': skipping collision creation (large ground plane)", 
                        object.name.c_str());
                collisionSkippedCount++;
                continue;
            }
            
            TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Plane collision: size=(%.2f, %.2f, %.2f)",
                     colliderSize.x, colliderSize.y, colliderSize.z);
            break;
        case MapObjectType::MODEL:
            // For models, build BVH collision using actual mesh data when available
            {
                const Model* modelPtr = nullptr;

                if (!object.modelName.empty())
                {
                    // Try runtime ModelLoader first (shared resource)
                    if (m_models)
                    {
                        auto modelOpt = m_models->GetModelByName(object.modelName);
                        if (!modelOpt)
                        {
                            // Try stem (filename without extension)
                            std::string stem = std::filesystem::path(object.modelName).stem().string();
                            if (!stem.empty())
                            {
                                modelOpt = m_models->GetModelByName(stem);
                            }
                        }

                        if (modelOpt)
                        {
                            modelPtr = &modelOpt->get();
                        }
                    }

                    // Fallback to models preloaded by MapLoader but not registered in ModelLoader yet
                    if (!modelPtr)
                    {
                        auto mapModelIt = m_gameMap.GetMapModels().find(object.modelName);
                        if (mapModelIt == m_gameMap.GetMapModels().end())
                        {
                            std::string stem = std::filesystem::path(object.modelName).stem().string();
                            if (!stem.empty())
                            {
                                mapModelIt = m_gameMap.GetMapModels().find(stem);
                            }
                        }

                        if (mapModelIt != m_gameMap.GetMapModels().end())
                        {
                            modelPtr = &mapModelIt->second;
                        }
                    }
                }

                if (modelPtr)
                {
                    Matrix translation = MatrixTranslate(object.position.x, object.position.y, object.position.z);
                    Matrix scaleMatrix = MatrixScale(object.scale.x, object.scale.y, object.scale.z);
                    Vector3 rotationRad = {
                        object.rotation.x * DEG2RAD,
                        object.rotation.y * DEG2RAD,
                        object.rotation.z * DEG2RAD
                    };
                    Matrix rotationMatrix = MatrixRotateXYZ(rotationRad);

                    Matrix transform = MatrixMultiply(scaleMatrix, MatrixMultiply(rotationMatrix, translation));

                    collision = Collision();
                    collision.BuildFromModelWithType(const_cast<Model *>(modelPtr), CollisionType::BVH_ONLY, transform);
                    useBVHCollision = true;

                    BoundingBox bb = collision.GetBoundingBox();
                    colliderSize = Vector3{
                        std::abs(bb.max.x - bb.min.x),
                        std::abs(bb.max.y - bb.min.y),
                        std::abs(bb.max.z - bb.min.z)
                    };

                    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Built BVH collision for model '%s'", object.modelName.c_str());
                }
                else
                {
                    colliderSize = Vector3{
                        std::abs(object.scale.x != 0.0f ? object.scale.x : 1.0f),
                        std::abs(object.scale.y != 0.0f ? object.scale.y : 1.0f),
                        std::abs(object.scale.z != 0.0f ? object.scale.z : 1.0f)
                    };
                    TraceLog(LOG_WARNING,
                             "MapManager::LoadEditorMap() - Model '%s' not found in ModelLoader or cached map models, using scale as collision size (AABB fallback)",
                             object.modelName.c_str(), colliderSize.x, colliderSize.y, colliderSize.z);
                }
            }
            break;
        case MapObjectType::LIGHT:
            // LIGHT objects don't need collision - they are just lighting
            TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - LIGHT object: skipping collision creation");
            collisionSkippedCount++;
            continue; // Skip collision creation for LIGHT objects
        default:
            // For unknown types, use absolute values of scale
            {
                colliderSize = Vector3{
                    std::abs(object.scale.x != 0.0f ? object.scale.x : 1.0f),
                    std::abs(object.scale.y != 0.0f ? object.scale.y : 1.0f),
                    std::abs(object.scale.z != 0.0f ? object.scale.z : 1.0f)
                };
            }
            TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Unknown type collision: size=(%.2f, %.2f, %.2f)",
                     colliderSize.x, colliderSize.y, colliderSize.z);
            break;
        }

        if (!useBVHCollision)
        {
            // Ensure colliderSize has valid non-zero dimensions (already handled above, but double-check)
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
                TraceLog(LOG_WARNING,
                         "MapManager::LoadEditorMap() - Object %d has invalid colliderSize after "
                         "calculation, skipping collision",
                         i);
                continue;
            }

            TraceLog(LOG_INFO,
                     "MapManager::LoadEditorMap() - Final colliderSize for object %d (AABB): (%.2f, %.2f, %.2f)", i,
                     colliderSize.x, colliderSize.y, colliderSize.z);

            // Collision constructor expects halfSize (half the dimensions)
            Vector3 halfSize = Vector3Scale(colliderSize, 0.5f);
            collision = Collision(object.position, halfSize);
            collision.SetCollisionType(CollisionType::AABB_ONLY);
        }
        else
        {
            // BVH collision already built; ensure type is BVH
            collision.SetCollisionType(CollisionType::BVH_ONLY);
        }

        m_collisionManager->AddCollider(std::move(collision));

        TraceLog(LOG_INFO,
                 "MapManager::LoadEditorMap() - Added collision for %s at (%.2f, %.2f, %.2f) with "
                 "size (%.2f, %.2f, %.2f) (type: %s)",
                 object.name.c_str(), object.position.x, object.position.y, object.position.z,
                 colliderSize.x, colliderSize.y, colliderSize.z,
                 useBVHCollision ? "BVH" : "AABB");
        collisionCreationCount++;
    }

    // Create player spawn zone from startPosition if specified in map metadata
    if (m_gameMap.GetMapMetaData().startPosition.x != 0.0f || m_gameMap.GetMapMetaData().startPosition.y != 0.0f ||
        m_gameMap.GetMapMetaData().startPosition.z != 0.0f)
    {
        // Create BoundingBox for spawn zone (2x2x2 units around start position)
        const float spawnSize = 2.0f;
        Vector3 spawnPos = m_gameMap.GetMapMetaData().startPosition;
        m_playerSpawnZone.min = {spawnPos.x - spawnSize/2, spawnPos.y - spawnSize/2, spawnPos.z - spawnSize/2};
        m_playerSpawnZone.max = {spawnPos.x + spawnSize/2, spawnPos.y + spawnSize/2, spawnPos.z + spawnSize/2};
        m_hasSpawnZone = true;
        
        TraceLog(LOG_INFO,
                 "MapManager::LoadEditorMap() - Created player spawn zone at (%.2f, %.2f, %.2f) size: %.2f",
                 spawnPos.x, spawnPos.y, spawnPos.z, spawnSize);
    }
    else
    {
        m_hasSpawnZone = false;
    }

    // Initialize collision manager after adding all colliders
    // This is crucial for collision detection to work properly
    m_collisionManager->Initialize();
    
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Successfully loaded map with %d objects",
             m_gameMap.GetMapObjects().size());
    TraceLog(LOG_INFO,
             "MapManager::LoadEditorMap() - Collision creation summary: %zu created, %zu skipped",
             collisionCreationCount, collisionSkippedCount);
    TraceLog(LOG_INFO,
             "MapManager::LoadEditorMap() - Final collider count after creating collisions: %zu",
             m_collisionManager->GetColliders().size());
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Collision manager initialized with all colliders");

    // Log object types breakdown
    int modelObjects = 0, lightObjects = 0, cubeObjects = 0, otherObjects = 0;
    for (const auto &obj : m_gameMap.GetMapObjects())
    {
        if (obj.type == MapObjectType::MODEL)
            modelObjects++;
        else if (obj.type == MapObjectType::LIGHT)
            lightObjects++;
        else if (obj.type == MapObjectType::CUBE)
            cubeObjects++;
        else
            otherObjects++;
    }
    TraceLog(LOG_INFO,
             "MapManager::LoadEditorMap() - Object types: %d MODEL, %d LIGHT, %d CUBE, %d other",
             modelObjects, lightObjects, cubeObjects, otherObjects);

    // Validate critical resources
    if (modelObjects > 0 && m_gameMap.GetMapModels().empty())
    {
        TraceLog(
            LOG_WARNING,
            "MapManager::LoadEditorMap() - WARNING: Map has %d model objects but no models were loaded!",
            modelObjects);
    }

    if (collisionCreationCount == 0)
    {
        TraceLog(LOG_ERROR, "MapManager::LoadEditorMap() - CRITICAL: No collisions were created for the "
                            "map! Physics will not work.");
    }

    // Dump diagnostics to help find why instances are not created
    DumpMapDiagnostics();

    // Create model instances in the ModelLoader for all MODEL objects so they are drawn
    TraceLog(LOG_INFO,
             "MapManager::LoadEditorMap() - Creating model instances for %d objects if applicable",
             m_gameMap.GetMapObjects().size());
    // First, ensure that all referenced model files are registered in the ModelLoader.
    // Some maps may reference model names (stems) or filenames with extensions — try common
    // extensions.
    std::set<std::string> uniqueModelNames;
    for (const auto &object : m_gameMap.GetMapObjects())
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
            uniqueModelNames.insert(object.modelName);
    }

    auto available = m_models->GetAvailableModels();
    for (const auto &requested : uniqueModelNames)
    {
        if (std::find(available.begin(), available.end(), requested) != available.end())
            continue; // already present

        // Try stem (strip extension)
        std::string stem = std::filesystem::path(requested).stem().string();
        if (!stem.empty() && std::find(available.begin(), available.end(), stem) != available.end())
            continue; // present as stem

        // Attempt to auto-load from resources using robust path resolution
        std::vector<std::string> possiblePaths;
        std::string extension = std::filesystem::path(requested).extension().string();

        if (extension.empty())
        {
            // No extension provided, try common extensions
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
            // Extension provided, use as-is
            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + requested);
            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + stem +
                                    extension);
        }

        bool loaded = false;
        for (const auto &resourcePath : possiblePaths)
        {
            if (std::ifstream(resourcePath).good())
            {
                TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Auto-loading candidate: %s",
                         resourcePath.c_str());
                if (m_models->LoadSingleModel(stem.empty() ? requested : stem, resourcePath, true))
                {
                    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Auto-loaded model '%s' from %s",
                             (stem.empty() ? requested : stem).c_str(), resourcePath.c_str());
                    loaded = true;
                    break;
                }
            }
        }

        if (!loaded)
        {
            TraceLog(LOG_WARNING,
                     "MapManager::LoadEditorMap() - Failed to auto-load model referenced by map: %s",
                     requested.c_str());
        }
    }

    // Refresh available list (some models may have been loaded)
    available = m_models->GetAvailableModels();
    for (const auto &object : m_gameMap.GetMapObjects())
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
        {
            // Ensure the model exists in ModelLoader; try fallbacks (stem, auto-load) if missing
            std::string requested = object.modelName;
            auto available = m_models->GetAvailableModels();
            bool exists =
                (std::find(available.begin(), available.end(), requested) != available.end());
            std::string candidateName = requested;

            if (!exists)
            {
                // Try filename stem (strip extension) — many configs use filenames while
                // ModelLoader stores names without extensions
                std::string stem = std::filesystem::path(requested).stem().string();
                if (!stem.empty() &&
                    std::find(available.begin(), available.end(), stem) != available.end())
                {
                    candidateName = stem;
                    exists = true;
                }
                else
                {
                    // Attempt to auto-load from resources using robust path resolution
                    std::vector<std::string> possiblePaths;
                    std::string extension = std::filesystem::path(requested).extension().string();

                    if (extension.empty())
                    {
                        // No extension provided, try common extensions
                        std::vector<std::string> extensions = {".glb", ".gltf", ".obj", ".fbx"};
                        for (const auto &ext : extensions)
                        {
                            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                    requested + ext);
                            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                    stem + ext);
                        }
                    }
                    else
                    {
                        // Extension provided, use as-is
                        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                requested);
                        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                stem + extension);
                    }

                    bool loaded = false;
                    for (const auto &resourcePath : possiblePaths)
                    {
                        if (std::ifstream(resourcePath).good())
                        {
                            TraceLog(LOG_INFO,
                                     "MapManager::LoadEditorMap() - Attempting to load model file for "
                                     "instance: %s from %s",
                                     requested.c_str(), resourcePath.c_str());

                            if (!stem.empty() &&
                                m_models->LoadSingleModel(stem, resourcePath, true))
                            {
                                candidateName = stem;
                                exists = true;
                                loaded = true;
                                TraceLog(
                                    LOG_INFO,
                                    "MapManager::LoadEditorMap() - Auto-loaded model as '%s' from %s",
                                    stem.c_str(), resourcePath.c_str());
                                break;
                            }
                            else if (m_models->LoadSingleModel(requested, resourcePath, true))
                            {
                                candidateName = requested;
                                exists = true;
                                loaded = true;
                                TraceLog(
                                    LOG_INFO,
                                    "MapManager::LoadEditorMap() - Auto-loaded model as '%s' from %s",
                                    requested.c_str(), resourcePath.c_str());
                                break;
                            }
                        }
                    }

                    if (!loaded)
                    {
                        TraceLog(LOG_WARNING,
                                 "MapManager::LoadEditorMap() - Could not auto-load model file for %s. "
                                 "Tried paths:",
                                 requested.c_str());
                        for (const auto &path : possiblePaths)
                        {
                            TraceLog(LOG_WARNING, "  - %s", path.c_str());
                        }
                    }
                }
            }

            if (!exists)
            {
                TraceLog(LOG_WARNING,
                         "MapManager::LoadEditorMap() - Model '%s' not available in ModelLoader; "
                         "skipping instance for object '%s'",
                         requested.c_str(), object.name.c_str());
                continue;
            }

            ModelInstanceConfig cfg;
            cfg.position = object.position;
            cfg.rotation = object.rotation;
            // Use uniform scale from X component, but handle Vector3 scale properly
            cfg.scale = (object.scale.x != 0.0f || object.scale.y != 0.0f || object.scale.z != 0.0f)
                            ? object.scale.x
                            : 1.0f; // Use X as uniform scale
            cfg.color = object.color;
            cfg.spawn = true;

            bool added = m_models->AddInstanceEx(candidateName, cfg);
            if (!added)
            {
                TraceLog(LOG_WARNING,
                         "MapManager::LoadEditorMap() - Failed to add instance for model '%s' (object "
                         "'%s') even after load attempts",
                         candidateName.c_str(), object.name.c_str());
            }
            else
            {
                TraceLog(
                    LOG_INFO,
                    "MapManager::LoadEditorMap() - Added instance for model '%s' at (%.2f, %.2f, %.2f)",
                    candidateName.c_str(), object.position.x, object.position.y, object.position.z);
            }
        }
        else if (object.type == MapObjectType::LIGHT)
        {
            // LIGHT objects are handled separately - no model instances needed
            TraceLog(
                LOG_INFO,
                "MapManager::LoadEditorMap() - Skipping LIGHT object '%s' for model instance creation",
                object.name.c_str());
        }
    }
}

void MapManager::RenderEditorMap()
{
    // Render the loaded map objects
    int renderedCount = 0;
    for (const auto &object : m_gameMap.GetMapObjects())
    {
        // Use color from map, or gray if color is black/zero (no color specified)
        Color renderColor = object.color;
        if (renderColor.a == 0)
        {
            renderColor.a = 255; // Make fully opaque if transparent
        }
        // If color is completely black/zero (no color specified), use gray
        if (renderColor.r == 0 && renderColor.g == 0 && renderColor.b == 0)
        {
            renderColor = GRAY; // Use gray for models without color
        }
        
        // Render based on object type
        switch (object.type)
        {
        case MapObjectType::CUBE:
        {
            // Ensure valid scale values
            float cubeWidth = (object.scale.x != 0.0f) ? object.scale.x : 1.0f;
            float cubeHeight = (object.scale.y != 0.0f) ? object.scale.y : 1.0f;
            float cubeLength = (object.scale.z != 0.0f) ? object.scale.z : 1.0f;
            DrawCube(object.position, cubeWidth, cubeHeight, cubeLength, renderColor);
            renderedCount++;
            break;
        }

        case MapObjectType::SPHERE:
        {
            // Ensure valid radius
            float sphereRadius = (object.radius > 0.0f) ? object.radius : 1.0f;
            DrawSphere(object.position, sphereRadius, renderColor);
            renderedCount++;
            break;
        }

        case MapObjectType::CYLINDER:
        {
            // Draw cylinder using proper DrawCylinder function
            float cylRadius = (object.radius > 0.0f) ? object.radius : 1.0f;
            float cylHeight = (object.height > 0.0f) ? object.height : 1.0f;
            DrawCylinder(object.position, cylRadius, cylRadius, cylHeight, 16, renderColor);
            renderedCount++;
            break;
        }

        case MapObjectType::PLANE:
        {
            // Draw plane as a thin cube
            float planeWidth = (object.size.x != 0.0f) ? object.size.x : 5.0f;
            float planeLength = (object.size.y != 0.0f) ? object.size.y : 5.0f;
            DrawPlane(object.position, Vector2{planeWidth, planeLength}, renderColor);
            renderedCount++;
            break;
        }

        case MapObjectType::MODEL:
        {
            // MODEL objects are rendered through ModelLoader instances via DrawAllModels()
            // in RenderGame() -> DrawScene3D(), so we don't render them here to avoid duplicates.
            // If model is not found or instance creation failed, it will show as missing
            // (instances are created in LoadEditorMap()).
            break;
        }

        case MapObjectType::LIGHT:
        {
            // LIGHT objects are not rendered as 3D models - they are lighting objects
            break;
        }

        case MapObjectType::SPAWN_ZONE:
        {
            // SPAWN_ZONE objects are rendered separately via RenderSpawnZone() (currently disabled)
            // Don't render them here as regular objects
            break;
        }

        default:
        {
            renderedCount = 0;
            break;
        }
        }
    }
    
    // Models are rendered through ModelLoader instances via DrawAllModels()
    // Only primitive objects are counted here
    
    // Render spawn zone if it exists
    // RenderSpawnZone(); // Disabled - spawn zone rendering turned off in game
}

Vector3 MapManager::GetPlayerSpawnPosition() const
{
    if (!m_hasSpawnZone)
    {
        return {0.0f, 0.0f, 0.0f};
    }
    
    return {
        (m_playerSpawnZone.min.x + m_playerSpawnZone.max.x) * 0.5f,
        (m_playerSpawnZone.min.y + m_playerSpawnZone.max.y) * 0.5f,
        (m_playerSpawnZone.min.z + m_playerSpawnZone.max.z) * 0.5f
    };
}

void MapManager::RenderSpawnZone() const
{
    if (!m_hasSpawnZone || !m_spawnTextureLoaded)
    {
        return;
    }
    
    // Calculate size and center of spawn zone
    Vector3 size = {
        m_playerSpawnZone.max.x - m_playerSpawnZone.min.x,
        m_playerSpawnZone.max.y - m_playerSpawnZone.min.y,
        m_playerSpawnZone.max.z - m_playerSpawnZone.min.z
    };
    
    Vector3 center = {
        (m_playerSpawnZone.min.x + m_playerSpawnZone.max.x) * 0.5f,
        (m_playerSpawnZone.min.y + m_playerSpawnZone.max.y) * 0.5f,
        (m_playerSpawnZone.min.z + m_playerSpawnZone.max.z) * 0.5f
    };
    
    // Use shared RenderUtils function to draw textured cube
    RenderUtils::DrawCubeTexture(m_spawnTexture, center, size.x, size.y, size.z, WHITE);
}

void MapManager::DumpMapDiagnostics() const
{
    TraceLog(LOG_INFO, "MapManager::DumpMapDiagnostics() - Map objects: %d", m_gameMap.GetMapObjects().size());

    for (size_t i = 0; i < m_gameMap.GetMapObjects().size(); ++i)
    {
        const auto &o = m_gameMap.GetMapObjects()[i];
        TraceLog(LOG_INFO,
                 "MapManager::DumpMapDiagnostics() - Object %d: name='%s' type=%d modelName='%s' "
                 "pos=(%.2f,%.2f,%.2f) scale=(%.2f,%.2f,%.2f)",
                 i, o.name.c_str(), static_cast<int>(o.type), o.modelName.c_str(), o.position.x,
                 o.position.y, o.position.z, o.scale.x, o.scale.y, o.scale.z);
    }

    // If MapLoader preloaded models into the map, list them
    if (!m_gameMap.GetMapModels().empty())
    {
        TraceLog(LOG_INFO, "MapManager::DumpMapDiagnostics() - GameMap.loadedModels contains %d entries",
                 m_gameMap.GetMapModels().size());
        for (const auto &p : m_gameMap.GetMapModels())
        {
            TraceLog(LOG_INFO, "MapManager::DumpMapDiagnostics() -   loadedModel key: %s (meshCount: %d)",
                     p.first.c_str(), p.second.meshCount);
        }
    }
    else
    {
        TraceLog(LOG_INFO, "MapManager::DumpMapDiagnostics() - GameMap.loadedModels is empty");
    }

    // List ModelLoader's available models
    auto available = m_models->GetAvailableModels();
    TraceLog(LOG_INFO, "MapManager::DumpMapDiagnostics() - ModelLoader available models: %d",
             available.size());
    for (const auto &name : available)
    {
        TraceLog(LOG_INFO, "MapManager::DumpMapDiagnostics() -   %s", name.c_str());
    }
}

void MapManager::InitCollisions()
{
    TraceLog(LOG_INFO, "MapManager::InitCollisions() - Initializing collision system...");

    // Only clear existing colliders if no custom map is loaded
    // If map is loaded, LoadEditorMap() has already created colliders for map objects
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && m_gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisions() - Clearing %zu existing colliders (no map loaded)",
                 previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !m_gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisions() - Map loaded with %zu existing colliders, preserving them",
                 previousColliderCount);
    }

    // Ground is now provided by map objects, no artificial ground needed
    if (m_gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisions() - No custom map loaded, no ground will be created");
    }
    else
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisions() - Custom map loaded, using map's ground objects");
    }

    // Initialize ground collider first
    m_collisionManager->Initialize();
    
  

    // Load model collisions only for models that are actually loaded and required for this map
    auto availableModels = m_models->GetAvailableModels();
    TraceLog(LOG_INFO, "MapManager::InitCollisions() - Available models for collision generation: %d",
             availableModels.size());
    for (const auto &modelName : availableModels)
    {
        TraceLog(LOG_INFO, "MapManager::InitCollisions() - Model available: %s", modelName.c_str());
    }

    m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, availableModels);
    TraceLog(LOG_INFO, "MapManager::InitCollisions() - Model collisions created");

    // Reinitialize after adding all model colliders
    m_collisionManager->Initialize();

    // Initialize player collision (if player is available)
    if (m_player) {
        auto &playerCollision = m_player->GetCollisionMutable();
        playerCollision.InitializeCollision();
    } else {
        TraceLog(LOG_WARNING, "MapManager::InitCollisions() - Player not available, skipping player collision initialization");
    }

    TraceLog(LOG_INFO, "MapManager::InitCollisions() - Collision system initialized with %zu colliders.",
             m_collisionManager->GetColliders().size());
}

void MapManager::InitCollisionsWithModels(const std::vector<std::string> &requiredModels)
{
    TraceLog(LOG_INFO,
             "MapManager::InitCollisionsWithModels() - Initializing collision system with %d required "
             "models...",
             requiredModels.size());

    // Only clear existing colliders if no custom map is loaded
    // If map is loaded, LoadEditorMap() has already created colliders for map objects
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && m_gameMap.GetMapObjects().empty())
    {
        TraceLog(
            LOG_INFO,
            "MapManager::InitCollisionsWithModels() - Clearing %zu existing colliders (no map loaded)",
            previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !m_gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisionsWithModels() - Map loaded with %zu existing colliders, "
                 "preserving them",
                 previousColliderCount);
    }

    // Initialize ground collider first
    m_collisionManager->Initialize();
    

    // Try to create model collisions, but don't fail if it doesn't work
    TraceLog(LOG_INFO,
             "MapManager::InitCollisionsWithModels() - Required models for collision generation: %d",
             requiredModels.size());
    for (const auto &modelName : requiredModels)
    {
        TraceLog(LOG_INFO, "MapManager::InitCollisionsWithModels() - Model required: %s",
                 modelName.c_str());
    }

    m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, requiredModels);
    TraceLog(LOG_INFO, "MapManager::InitCollisionsWithModels() - Model collisions created");

    // Reinitialize after adding all model colliders
    m_collisionManager->Initialize();

    // Initialize player collision (if player is available)
    if (m_player) {
        auto &playerCollision = m_player->GetCollisionMutable();
        playerCollision.InitializeCollision();
    } else {
        TraceLog(LOG_WARNING, "MapManager::InitCollisionsWithModels() - Player not available, skipping player collision initialization");
    }

    TraceLog(LOG_INFO,
             "MapManager::InitCollisionsWithModels() - Collision system initialized with %zu colliders.",
             m_collisionManager->GetColliders().size());
}

bool MapManager::InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels)
{
    TraceLog(LOG_INFO,
             "MapManager::InitCollisionsWithModelsSafe() - Initializing collision system with %d "
             "required models...",
             requiredModels.size());

    // Only clear existing colliders if no custom map is loaded
    // If map is loaded, LoadEditorMap() has already created colliders for map objects
    size_t previousColliderCount = m_collisionManager->GetColliders().size();
    if (previousColliderCount > 0 && m_gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisionsWithModelsSafe() - Clearing %zu existing colliders (no map "
                 "loaded)",
                 previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !m_gameMap.GetMapObjects().empty())
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisionsWithModelsSafe() - Map loaded with %zu existing colliders, "
                 "preserving them",
                 previousColliderCount);
    }

    // Initialize collision manager
    m_collisionManager->Initialize();

    // Try to create model collisions, but don't fail if it doesn't work
    TraceLog(LOG_INFO,
             "MapManager::InitCollisionsWithModelsSafe() - Required models for collision generation: %d",
             requiredModels.size());
    for (const auto &modelName : requiredModels)
    {
        TraceLog(LOG_INFO, "MapManager::InitCollisionsWithModelsSafe() - Model required: %s",
                 modelName.c_str());
    }

    // Try to create model collisions safely
    m_collisionManager->CreateAutoCollisionsFromModelsSelective(*m_models, requiredModels);
    TraceLog(LOG_INFO, "MapManager::InitCollisionsWithModelsSafe() - Model collisions created");
    
    // Reinitialize after adding all model colliders
    m_collisionManager->Initialize();

    // Initialize player collision (if player is available)
    if (m_player) {
        auto &playerCollision = m_player->GetCollisionMutable();
        playerCollision.InitializeCollision();
    } else {
        TraceLog(LOG_WARNING, "MapManager::InitCollisionsWithModelsSafe() - Player not available, skipping player collision initialization");
    }

    TraceLog(
        LOG_INFO,
        "MapManager::InitCollisionsWithModelsSafe() - Collision system initialized with %zu colliders.",
        m_collisionManager->GetColliders().size());

    return true; // Always return true since we have at least basic collision
}

void MapManager::SetPlayer(Player* player)
{
    m_player = player;
    TraceLog(LOG_INFO, "MapManager::SetPlayer() - Player reference updated");
}
