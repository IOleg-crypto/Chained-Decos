#include "MapManager.h"
#include "Player/Player.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Render/RenderManager.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "Engine/MapFileManager/JsonMapFileManager.h"
#include "Game/Menu/Menu.h"
#include "Player/PlayerCollision.h"
#include <raylib.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <unordered_set>

MapManager::MapManager(Player* player, CollisionManager* collisionManager, ModelLoader* models, RenderManager* renderManager, Kernel* kernel, Menu* menu)
    : m_player(player), m_collisionManager(collisionManager), m_models(models), m_renderManager(renderManager), m_kernel(kernel), m_menu(menu)
{
    TraceLog(LOG_INFO, "MapManager created");
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

    // Clear previous map data
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Clearing previous map data...");
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Current collider count before map load: %zu",
             m_collisionManager->GetColliders().size());

    // Check if this is a JSON file exported from map editor
    std::string extension = mapPath.substr(mapPath.find_last_of(".") + 1);
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - File extension: %s", extension.c_str());

    if (extension == "json")
    {
        TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Detected JSON format, using MapLoader");

        TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Using MapEditor loader (JsonMapFileManager) "
                           "for JSON parsing...");
        // Use Map Editor's loader directly and convert its objects into GameMap
        std::vector<JsonSerializableObject> editorObjects;
        MapMetadata editorMeta;
        if (!JsonMapFileManager::LoadMap(editorObjects, mapPath, editorMeta))
        {
            TraceLog(LOG_ERROR, "MapManager::LoadEditorMap() - JsonMapFileManager failed to load map: %s",
                     mapPath.c_str());
            TraceLog(LOG_ERROR, "MapManager::LoadEditorMap() - This may indicate JSON parsing errors or "
                                "invalid map format");
            return;
        }

        TraceLog(LOG_INFO,
                 "MapManager::LoadEditorMap() - JsonMapFileManager loaded %zu objects successfully",
                 editorObjects.size());

        GameMap adapterMap;
        adapterMap.metadata = editorMeta;

        for (const auto &eo : editorObjects)
        {
            TraceLog(LOG_INFO,
                     "MapManager::LoadEditorMap() - Processing editor object: name='%s', type=%d, "
                     "modelName='%s'",
                     eo.name.c_str(), eo.type, eo.modelName.c_str());

            MapObjectData od;
            od.name =
                eo.name.empty() ? ("object_" + std::to_string(adapterMap.objects.size())) : eo.name;
            od.type = static_cast<MapObjectType>(eo.type);
            od.position = eo.position;
            
            if(od.name.find("player") != std::string::npos)
            {
                m_player->SetPlayerPosition(od.position);
            }
            od.rotation = eo.rotation;
            od.scale = eo.scale;
            od.color = eo.color;
            od.modelName = eo.modelName;
            od.radius = eo.radiusSphere;
            od.height = eo.radiusV;
            od.size = eo.size;
            od.isPlatform = true;
            od.isObstacle = false;

            // Sanitize values coming from editor JSON to avoid NaN/Inf propagating to render
            auto sanitizeVec3 = [](Vector3 v)
            {
                if (!std::isfinite(v.x))
                    v.x = 0.0f;
                if (!std::isfinite(v.y))
                    v.y = 0.0f;
                if (!std::isfinite(v.z))
                    v.z = 0.0f;
                return v;
            };
            auto sanitizeFloat = [](float f, float fallback)
            { return std::isfinite(f) ? f : fallback; };

            od.position = sanitizeVec3(od.position);
            od.rotation = sanitizeVec3(od.rotation);
            od.scale = sanitizeVec3(od.scale);
            if (od.scale.x == 0.0f && od.scale.y == 0.0f && od.scale.z == 0.0f)
            {
                od.scale = {1.0f, 1.0f, 1.0f};
            }
            od.radius = sanitizeFloat(od.radius, 1.0f);
            od.height = sanitizeFloat(od.height, 1.0f);
            if (!std::isfinite(od.size.x))
                od.size.x = 0.0f;
            if (!std::isfinite(od.size.y))
                od.size.y = 0.0f;

            TraceLog(LOG_INFO,
                     "MapManager::LoadEditorMap() - Sanitized object: pos=(%.2f,%.2f,%.2f), "
                     "scale=(%.2f,%.2f,%.2f), radius=%.2f",
                     od.position.x, od.position.y, od.position.z, od.scale.x, od.scale.y,
                     od.scale.z, od.radius);

            adapterMap.objects.push_back(od);

            // If this object references a model, attempt to preload it into the
            // GameMap.loadedModels
            if (od.type == MapObjectType::MODEL && !od.modelName.empty())
            {
                TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Attempting to load model: %s",
                         od.modelName.c_str());

                // Try multiple path variations for the model
                std::vector<std::string> possiblePaths;
                std::string modelName = od.modelName;

                // Remove extension if present to try different extensions
                std::string stem = std::filesystem::path(modelName).stem().string();
                std::string extension = std::filesystem::path(modelName).extension().string();

                // Build possible paths
                if (extension.empty())
                {
                    // No extension provided, try common extensions
                    std::vector<std::string> extensions = {".glb", ".gltf", ".obj", ".fbx"};
                    for (const auto &ext : extensions)
                    {
                        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                modelName + ext);
                        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                                stem + ext);
                    }
                }
                else
                {
                    // Extension provided, use as-is
                    possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                            modelName);
                    possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + stem +
                                            extension);
                }

                TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Trying %zu possible paths for model %s",
                         possiblePaths.size(), od.modelName.c_str());

                // Try to find and load the model
                bool modelLoaded = false;
                for (const auto &modelPath : possiblePaths)
                {
                    TraceLog(LOG_DEBUG, "MapManager::LoadEditorMap() - Checking path: %s",
                             modelPath.c_str());
                    if (std::ifstream(modelPath).good())
                    {
                        TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Found model file at: %s",
                                 modelPath.c_str());
                        // Avoid duplicate loads
                        if (adapterMap.loadedModels.find(od.modelName) ==
                            adapterMap.loadedModels.end())
                        {
                            Model model = ::LoadModel(modelPath.c_str());
                            if (model.meshCount > 0)
                            {
                                adapterMap.loadedModels[od.modelName] = model;
                                TraceLog(LOG_INFO,
                                         "MapManager::LoadEditorMap() - Successfully loaded model %s "
                                         "from %s (meshCount: %d)",
                                         od.modelName.c_str(), modelPath.c_str(), model.meshCount);
                                modelLoaded = true;
                                break;
                            }
                            else
                            {
                                TraceLog(
                                    LOG_WARNING,
                                    "MapManager::LoadEditorMap() - Model loaded but has no meshes: %s",
                                    modelPath.c_str());
                            }
                        }
                        else
                        {
                            TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Model %s already loaded",
                                     od.modelName.c_str());
                            modelLoaded = true;
                            break;
                        }
                    }
                }

                if (!modelLoaded)
                {
                    TraceLog(LOG_ERROR,
                             "MapManager::LoadEditorMap() - CRITICAL: Could not find or load model file "
                             "for %s. This will cause rendering failures.",
                             od.modelName.c_str());
                    TraceLog(LOG_ERROR, "MapManager::LoadEditorMap() - Tried paths:");
                    for (const auto &path : possiblePaths)
                    {
                        TraceLog(LOG_ERROR, "  - %s", path.c_str());
                    }
                }
            }
        }

        m_gameMap = std::move(adapterMap);
    m_currentMapPath = mapPath;

        if (!m_gameMap.objects.empty())
        {
            // Register any models preloaded by MapLoader into the runtime ModelLoader
            if (!m_gameMap.loadedModels.empty())
            {
                TraceLog(LOG_INFO,
                         "MapManager::LoadEditorMap() - Registering %d preloaded models from map into "
                         "ModelLoader",
                         m_gameMap.loadedModels.size());
                for (const auto &p : m_gameMap.loadedModels)
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
                     "MapManager::LoadEditorMap() - MapLoader import successful, processing %d objects",
                     m_gameMap.objects.size());

            TraceLog(LOG_INFO,
                     "MapManager::LoadEditorMap() - Successfully loaded JSON map with %d objects",
                     m_gameMap.objects.size());
            TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Debug: Object count verified as %d",
                     m_gameMap.objects.size());
        }
        else
        {
            TraceLog(LOG_ERROR, "MapManager::LoadEditorMap() - Failed to load JSON map");
            return;
        }
    }

    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Map loaded, checking object count: %d",
             m_gameMap.objects.size());
    if (m_gameMap.objects.empty())
    {
        TraceLog(LOG_ERROR, "MapManager::LoadEditorMap() - No objects loaded from map");
        return;
    }

    // Validate map object count to prevent memory issues
    if (m_gameMap.objects.size() > 10000)
    {
        TraceLog(LOG_ERROR,
                 "MapManager::LoadEditorMap() - Map has too many objects (%d), limiting to 10000",
                 m_gameMap.objects.size());
        return;
    }

    // Create collision boxes for all objects in the map
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Creating collision boxes for %d objects",
             m_gameMap.objects.size());
    size_t collisionCreationCount = 0;
    size_t collisionSkippedCount = 0;

    for (size_t i = 0; i < m_gameMap.objects.size(); ++i)
    {
        const auto &object = m_gameMap.objects[i];

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
            // For models, use absolute values of scale as bounding box
            {
                colliderSize = Vector3{
                    std::abs(object.scale.x != 0.0f ? object.scale.x : 1.0f),
                    std::abs(object.scale.y != 0.0f ? object.scale.y : 1.0f),
                    std::abs(object.scale.z != 0.0f ? object.scale.z : 1.0f)
                };
            }
            TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Model collision: size=(%.2f, %.2f, %.2f)",
                     colliderSize.x, colliderSize.y, colliderSize.z);
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
                 "MapManager::LoadEditorMap() - Final colliderSize for object %d: (%.2f, %.2f, %.2f)", i,
                 colliderSize.x, colliderSize.y, colliderSize.z);

        // Collision constructor expects halfSize (half the dimensions), not full size
        // So we need to divide by 2 to get the correct collision box size
        Vector3 halfSize = Vector3Scale(colliderSize, 0.5f);
        Collision collision(object.position, halfSize);
        collision.SetCollisionType(CollisionType::AABB_ONLY);
        m_collisionManager->AddCollider(std::move(collision));

        TraceLog(LOG_INFO,
                 "MapManager::LoadEditorMap() - Added collision for %s at (%.2f, %.2f, %.2f) with "
                 "size (%.2f, %.2f, %.2f)",
                 object.name.c_str(), object.position.x, object.position.y, object.position.z,
                 colliderSize.x, colliderSize.y, colliderSize.z);
        collisionCreationCount++;
    }

    // Set player start position if specified in map metadata
    if (m_gameMap.metadata.startPosition.x != 0.0f || m_gameMap.metadata.startPosition.y != 0.0f ||
        m_gameMap.metadata.startPosition.z != 0.0f)
    {
        m_player->SetPlayerPosition(m_gameMap.metadata.startPosition);
        TraceLog(LOG_INFO,
                 "MapManager::LoadEditorMap() - Set player start position to (%.2f, %.2f, %.2f)",
                 m_gameMap.metadata.startPosition.x, m_gameMap.metadata.startPosition.y,
                 m_gameMap.metadata.startPosition.z);
    }

    // Initialize collision manager after adding all colliders
    // This is crucial for collision detection to work properly
    m_collisionManager->Initialize();
    
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Successfully loaded map with %d objects",
             m_gameMap.objects.size());
    TraceLog(LOG_INFO,
             "MapManager::LoadEditorMap() - Collision creation summary: %zu created, %zu skipped",
             collisionCreationCount, collisionSkippedCount);
    TraceLog(LOG_INFO,
             "MapManager::LoadEditorMap() - Final collider count after creating collisions: %zu",
             m_collisionManager->GetColliders().size());
    TraceLog(LOG_INFO, "MapManager::LoadEditorMap() - Collision manager initialized with all colliders");

    // Log object types breakdown
    int modelObjects = 0, lightObjects = 0, cubeObjects = 0, otherObjects = 0;
    for (const auto &obj : m_gameMap.objects)
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
    if (modelObjects > 0 && m_gameMap.loadedModels.empty())
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
             m_gameMap.objects.size());
    // First, ensure that all referenced model files are registered in the ModelLoader.
    // Some maps may reference model names (stems) or filenames with extensions — try common
    // extensions.
    std::set<std::string> uniqueModelNames;
    for (const auto &object : m_gameMap.objects)
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
    for (const auto &object : m_gameMap.objects)
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
    for (const auto &object : m_gameMap.objects)
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
            // Draw cylinder using multiple spheres for approximation
            // For better cylinder rendering, you might want to use a 3D model
            float cylRadius = (object.radius > 0.0f) ? object.radius : 1.0f;
            DrawSphere(object.position, cylRadius, renderColor);
            DrawSphere(
                Vector3{object.position.x, object.position.y + object.height, object.position.z},
                cylRadius, renderColor);
            renderedCount++;
            break;
        }

        case MapObjectType::PLANE:
        {
            // Draw plane as a thin cube
            float planeWidth = (object.size.x != 0.0f) ? object.size.x : 5.0f;
            float planeLength = (object.size.y != 0.0f) ? object.size.y : 5.0f;
            DrawCube(object.position, planeWidth, 0.1f, planeLength, renderColor);
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

        default:
        {
            // Unknown type, draw as cube
            float defWidth = (object.scale.x != 0.0f) ? object.scale.x : 1.0f;
            float defHeight = (object.scale.y != 0.0f) ? object.scale.y : 1.0f;
            float defLength = (object.scale.z != 0.0f) ? object.scale.z : 1.0f;
            DrawCube(object.position, defWidth, defHeight, defLength, renderColor);
            renderedCount++;
            break;
        }
        }
    }
    
    TraceLog(LOG_INFO, "MapManager::RenderEditorMap() - Rendered %d primitive objects out of %zu total objects",
             renderedCount, m_gameMap.objects.size());
}

void MapManager::DumpMapDiagnostics() const
{
    TraceLog(LOG_INFO, "MapManager::DumpMapDiagnostics() - Map objects: %d", m_gameMap.objects.size());

    for (size_t i = 0; i < m_gameMap.objects.size(); ++i)
    {
        const auto &o = m_gameMap.objects[i];
        TraceLog(LOG_INFO,
                 "MapManager::DumpMapDiagnostics() - Object %d: name='%s' type=%d modelName='%s' "
                 "pos=(%.2f,%.2f,%.2f) scale=(%.2f,%.2f,%.2f)",
                 i, o.name.c_str(), static_cast<int>(o.type), o.modelName.c_str(), o.position.x,
                 o.position.y, o.position.z, o.scale.x, o.scale.y, o.scale.z);
    }

    // If MapLoader preloaded models into the map, list them
    if (!m_gameMap.loadedModels.empty())
    {
        TraceLog(LOG_INFO, "MapManager::DumpMapDiagnostics() - GameMap.loadedModels contains %d entries",
                 m_gameMap.loadedModels.size());
        for (const auto &p : m_gameMap.loadedModels)
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
    if (previousColliderCount > 0 && m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisions() - Clearing %zu existing colliders (no map loaded)",
                 previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisions() - Map loaded with %zu existing colliders, preserving them",
                 previousColliderCount);
    }

    // Ground is now provided by map objects, no artificial ground needed
    if (m_gameMap.objects.empty())
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
    
    // Register collision service once initialized
    if (m_kernel)
    {
        m_kernel->RegisterService<CollisionService>(
            Kernel::ServiceType::Collision,
            std::make_shared<CollisionService>(m_collisionManager));
    }

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

    // Initialize player collision
    auto &playerCollision = m_player->GetCollisionMutable();
    playerCollision.InitializeCollision();

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
    if (previousColliderCount > 0 && m_gameMap.objects.empty())
    {
        TraceLog(
            LOG_INFO,
            "MapManager::InitCollisionsWithModels() - Clearing %zu existing colliders (no map loaded)",
            previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisionsWithModels() - Map loaded with %zu existing colliders, "
                 "preserving them",
                 previousColliderCount);
    }

    // Initialize ground collider first
    m_collisionManager->Initialize();
    
    // Register collision service once initialized
    if (m_kernel)
    {
        m_kernel->RegisterService<CollisionService>(
            Kernel::ServiceType::Collision,
            std::make_shared<CollisionService>(m_collisionManager));
    }

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

    // Initialize player collision
    auto &playerCollision = m_player->GetCollisionMutable();
    playerCollision.InitializeCollision();

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
    if (previousColliderCount > 0 && m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisionsWithModelsSafe() - Clearing %zu existing colliders (no map "
                 "loaded)",
                 previousColliderCount);
        m_collisionManager->ClearColliders();
    }
    else if (previousColliderCount > 0 && !m_gameMap.objects.empty())
    {
        TraceLog(LOG_INFO,
                 "MapManager::InitCollisionsWithModelsSafe() - Map loaded with %zu existing colliders, "
                 "preserving them",
                 previousColliderCount);
    }

    // Initialize collision manager
    m_collisionManager->Initialize();

    // Register collision service once initialized
    if (m_kernel)
    {
        m_kernel->RegisterService<CollisionService>(
            Kernel::ServiceType::Collision,
            std::make_shared<CollisionService>(m_collisionManager));
    }

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

    // Initialize player collision
    auto &playerCollision = m_player->GetCollisionMutable();
    playerCollision.InitializeCollision();

    TraceLog(
        LOG_INFO,
        "MapManager::InitCollisionsWithModelsSafe() - Collision system initialized with %zu colliders.",
        m_collisionManager->GetColliders().size());

    return true; // Always return true since we have at least basic collision
}
