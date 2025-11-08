#include "MapLoader.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <raymath.h>
#include <set>


using json = nlohmann::json;

// Helper function to resolve model paths
std::vector<std::string> ResolveModelPaths(const std::string &modelName)
{
    std::vector<std::string> possiblePaths;
    std::string stem = std::filesystem::path(modelName).stem().string();
    std::string extension = std::filesystem::path(modelName).extension().string();

    // Normalize path separators to forward slashes for consistency
    std::string normalizedModelName = modelName;
    std::replace(normalizedModelName.begin(), normalizedModelName.end(), '\\', '/');

    if (extension.empty())
    {
        // No extension provided, try common extensions in multiple locations
        std::vector<std::string> extensions = {".glb", ".gltf", ".obj", ".fbx", ".dae"};

        // Try in resources/ directory
        for (const auto &ext : extensions)
        {
            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "resources/" +
                                    normalizedModelName + ext);
            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "resources/models/" +
                                    normalizedModelName + ext);
        }

        // Try with stem variations
        for (const auto &ext : extensions)
        {
            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "resources/" + stem + ext);
            possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "resources/models/" + stem +
                                    ext);
        }

        // Try absolute paths if modelName contains path separators
        if (normalizedModelName.find('/') != std::string::npos)
        {
            for (const auto &ext : extensions)
            {
                possiblePaths.push_back(normalizedModelName + ext);
                if (normalizedModelName[0] == '/')
                {
                    possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) +
                                            normalizedModelName.substr(1) + ext);
                }
            }
        }
    }
    else
    {
        // Extension provided, use as-is with multiple path variations
        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "resources/" + normalizedModelName);
        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "resources/models/" +
                                normalizedModelName);

        // Try with stem variation
        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "resources/" + stem + extension);
        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "resources/models/" + stem +
                                extension);

        // Try absolute path if provided
        if (normalizedModelName.find('/') != std::string::npos)
        {
            possiblePaths.push_back(normalizedModelName);
            if (normalizedModelName[0] == '/')
            {
                possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) +
                                        normalizedModelName.substr(1));
            }
        }
    }

    return possiblePaths;
}

// Helper function to load model with error handling
bool LoadModelWithErrorHandling(const std::string &modelName,
                                const std::vector<std::string> &possiblePaths,
                                std::unordered_map<std::string, Model> &loadedModels)
{
    // Normalize the model name key for consistent storage
    std::string normalizedKey = modelName;
    std::replace(normalizedKey.begin(), normalizedKey.end(), '\\', '/');

    // Remove any path components from the key, keep only the filename
    std::filesystem::path keyPath(normalizedKey);
    std::string keyStem = keyPath.stem().string();
    std::string keyExtension = keyPath.extension().string();

    // Create a clean key without path or extension
    std::string cleanKey = keyStem;
    if (!keyExtension.empty())
    {
        cleanKey += keyExtension;
    }

    for (const auto &modelPath : possiblePaths)
    {
        if (FileExists(modelPath.c_str()))
        {
            // Check if model is already loaded using the clean key
            if (loadedModels.find(cleanKey) == loadedModels.end())
            {
                Model model = LoadModel(modelPath.c_str());
                if (model.meshCount > 0)
                {
                    loadedModels[cleanKey] = model;
                    TraceLog(
                        LOG_INFO,
                        "MapLoader: Successfully loaded model %s (key: %s) from %s (meshCount: %d)",
                        modelName.c_str(), cleanKey.c_str(), modelPath.c_str(), model.meshCount);
                    return true;
                }
                else
                {
                    TraceLog(LOG_WARNING, "MapLoader: Model loaded but has no meshes: %s",
                             modelPath.c_str());
                }
            }
            else
            {
                TraceLog(LOG_INFO, "MapLoader: Model %s (key: %s) already loaded",
                         modelName.c_str(), cleanKey.c_str());
                return true;
            }
        }
    }

    TraceLog(LOG_WARNING,
             "MapLoader: Could not find model file for %s. Tried paths:", modelName.c_str());
    for (const auto &path : possiblePaths)
    {
        TraceLog(LOG_WARNING, "  - %s", path.c_str());
    }
    return false;
}

// GameMap struct implementation
void GameMap::Cleanup()
{
    for (auto &pair : m_loadedModels)
    {
        if (pair.second.meshCount > 0)
            UnloadModel(pair.second);
    }
    m_loadedModels.clear();

    // Cleanup skybox
    if (m_skybox)
    {
        m_skybox.reset();
    }
}

GameMap::~GameMap()
{
    Cleanup();
}

// ============================================================================
// New comprehensive map loading system
// ============================================================================

GameMap LoadGameMap(const std::string &path)
{
    GameMap map;

    if (!FileExists(path.c_str()))
    {
        TraceLog(LOG_ERROR, "Map file not found: %s", path.c_str());
        return map;
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        TraceLog(LOG_ERROR, "Failed to open map file: %s", path.c_str());
        return map;
    }

    json j;
    try
    {
        file >> j;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Failed to parse map JSON: %s", e.what());
        return map;
    }

    // Detect format: check if it's models.json format (array of models) or editor format (metadata
    // + objects)
    if (j.is_array() && !j.empty() && j[0].contains("name") && j[0].contains("instances"))
    {
        // This is models.json format
        return LoadGameMapFromModelsFormat(j, path);
    }
    else
    {
        // This is editor format with metadata and objects
        return LoadGameMapFromEditorFormat(j, path);
    }
}

GameMap LoadGameMapFromModelsFormat(const json &j, const std::string &path)
{
    GameMap map;

    // Load models from the models.json format
    for (const auto &modelData : j)
    {
        std::string modelName = modelData.value("name", "");
        std::string modelPath = modelData.value("path", "");

        // Load the model
        if (!modelPath.empty())
        {
            // Try multiple path variations for the model
            std::vector<std::string> possiblePaths;

            // Build possible paths
            if (modelPath.find('/') == std::string::npos &&
                modelPath.find('\\') == std::string::npos)
            {
                // Relative path, try in resources folder
                possiblePaths = ResolveModelPaths(modelPath);
            }
            else
            {
                // Absolute or relative path with separators
                possiblePaths.push_back(modelPath);
                if (modelPath[0] == '/')
                {
                    possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + modelPath);
                }
            }

            // Try to find and load the model
            if (!LoadModelWithErrorHandling(modelName, possiblePaths, map.m_loadedModels))
            {
                TraceLog(LOG_WARNING, "Model file not found for %s", modelName.c_str());
                continue;
            }
        }
        else
        {
            TraceLog(LOG_WARNING, "Empty model path for model: %s", modelName.c_str());
            continue;
        }

        // Load instances for this model
        if (modelData.contains("instances") && modelData["instances"].is_array())
        {
            for (const auto &instance : modelData["instances"])
            {
                MapObjectData objectData;

                // Set basic properties
                objectData.name = modelName + "_" + std::to_string(map.m_objects.size());
                objectData.type = MapObjectType::MODEL;
                objectData.modelName = modelName;

                // Load position
                if (instance.contains("position") && instance["position"].is_object())
                {
                    auto &pos = instance["position"];
                    objectData.position =
                        Vector3{pos.value("x", 0.0f), pos.value("y", 0.0f), pos.value("z", 0.0f)};
                }

                // Load scale
                float scaleValue = instance.value("scale", 1.0f);
                objectData.scale = Vector3{scaleValue, scaleValue, scaleValue};

                // Set default color
                objectData.color = WHITE;

                map.m_objects.push_back(objectData);
            }
        }
    }

    TraceLog(LOG_INFO, "Successfully loaded models.json format map: %s with %d objects",
             path.c_str(), map.m_objects.size());

    return map;
}

GameMap LoadGameMapFromEditorFormat(const json &j, const std::string &path)
{
    GameMap map;

    // Load metadata
    if (j.contains("metadata"))
    {
        const auto &meta = j["metadata"];
        map.m_metadata.name = meta.value("name", "unnamed_map");
        map.m_metadata.displayName = meta.value("displayName", "Unnamed Map");
        map.m_metadata.description = meta.value("description", "");
        map.m_metadata.author = meta.value("author", "");
        map.m_metadata.version = meta.value("version", "1.0");
        map.m_metadata.difficulty = meta.value("difficulty", 1.0f);

        // Load colors
        if (meta.contains("skyColor"))
        {
            auto &sky = meta["skyColor"];
            map.m_metadata.skyColor = Color{static_cast<unsigned char>(sky.value("r", 135)),
                                            static_cast<unsigned char>(sky.value("g", 206)),
                                            static_cast<unsigned char>(sky.value("b", 235)),
                                            static_cast<unsigned char>(sky.value("a", 255))};
        }

        if (meta.contains("groundColor"))
        {
            auto &ground = meta["groundColor"];
            map.m_metadata.groundColor = Color{static_cast<unsigned char>(ground.value("r", 34)),
                                               static_cast<unsigned char>(ground.value("g", 139)),
                                               static_cast<unsigned char>(ground.value("b", 34)),
                                               static_cast<unsigned char>(ground.value("a", 255))};
        }

        // Load positions
        if (meta.contains("startPosition"))
        {
            auto &start = meta["startPosition"];
            map.m_metadata.startPosition =
                Vector3{start.value("x", 0.0f), start.value("y", 0.0f), start.value("z", 0.0f)};
        }

        if (meta.contains("endPosition"))
        {
            auto &end = meta["endPosition"];
            map.m_metadata.endPosition =
                Vector3{end.value("x", 0.0f), end.value("y", 0.0f), end.value("z", 0.0f)};
        }

        // Load skybox texture path
        if (meta.contains("skyboxTexture"))
        {
            map.m_metadata.skyboxTexture = meta.value("skyboxTexture", "");
        }
    }

    // Load skybox if texture path is specified
    if (!map.m_metadata.skyboxTexture.empty())
    {
        LoadSkyboxForMap(map);
    }

    // Load objects
    if (j.contains("objects"))
    {
        for (const auto &obj : j["objects"])
        {
            MapObjectData objectData;

            // Basic properties
            objectData.name = obj.value("name", "object_" + std::to_string(map.m_objects.size()));
            objectData.type = static_cast<MapObjectType>(obj.value("type", 0));
            TraceLog(LOG_INFO, "MapLoader: Loading object %s, type %d", objectData.name.c_str(),
                     static_cast<int>(objectData.type));

            // Position
            if (obj.contains("position"))
            {
                auto &pos = obj["position"];
                objectData.position =
                    Vector3{pos.value("x", 0.0f), pos.value("y", 0.0f), pos.value("z", 0.0f)};
            }

            // Rotation
            if (obj.contains("rotation"))
            {
                auto &rot = obj["rotation"];
                objectData.rotation =
                    Vector3{rot.value("x", 0.0f), rot.value("y", 0.0f), rot.value("z", 0.0f)};
            }

            // Scale - ensure consistent handling
            if (obj.contains("scale"))
            {
                auto &scl = obj["scale"];
                objectData.scale =
                    Vector3{scl.value("x", 1.0f), scl.value("y", 1.0f), scl.value("z", 1.0f)};

                // Normalize scale values to prevent zero/negative scales
                objectData.scale.x = (objectData.scale.x <= 0.0f) ? 1.0f : objectData.scale.x;
                objectData.scale.y = (objectData.scale.y <= 0.0f) ? 1.0f : objectData.scale.y;
                objectData.scale.z = (objectData.scale.z <= 0.0f) ? 1.0f : objectData.scale.z;
            }
            else
            {
                // Default scale if not specified
                objectData.scale = Vector3{1.0f, 1.0f, 1.0f};
            }

            // Color
            if (obj.contains("color"))
            {
                auto &col = obj["color"];
                objectData.color = Color{static_cast<unsigned char>(col.value("r", 255)),
                                         static_cast<unsigned char>(col.value("g", 255)),
                                         static_cast<unsigned char>(col.value("b", 255)),
                                         static_cast<unsigned char>(col.value("a", 255))};
            }

            // Model name (for MODEL type)
            objectData.modelName = obj.value("modelName", "");

            // Shape-specific properties - ensure consistency with scale
            objectData.radius =
                obj.value("radius", objectData.scale.x); // Default to scale.x if not specified
            objectData.height =
                obj.value("height", objectData.scale.y); // Default to scale.y if not specified

            if (obj.contains("size"))
            {
                auto &sz = obj["size"];
                objectData.size = Vector2{
                    sz.value("width", objectData.scale.x), // Default to scale.x if not specified
                    sz.value("height", objectData.scale.z) // Default to scale.z if not specified
                };
            }
            else
            {
                // Set size based on scale for consistency
                objectData.size = Vector2{objectData.scale.x, objectData.scale.z};
            }

            // Collision properties
            objectData.isPlatform = obj.value("isPlatform", true);
            objectData.isObstacle = obj.value("isObstacle", false);

            map.m_objects.push_back(objectData);

            // Load model if it's a MODEL type object
            if (objectData.type == MapObjectType::MODEL && !objectData.modelName.empty())
            {
                TraceLog(LOG_INFO, "MapLoader: Loading MODEL object %s with modelName %s",
                         objectData.name.c_str(), objectData.modelName.c_str());

                // Use helper function to resolve paths and load model
                std::vector<std::string> possiblePaths = ResolveModelPaths(objectData.modelName);
                LoadModelWithErrorHandling(objectData.modelName, possiblePaths, map.m_loadedModels);
            }
            // Handle LIGHT type objects that may actually be misclassified MODEL objects from map
            // editor
            else if (objectData.type == MapObjectType::LIGHT && !objectData.modelName.empty())
            {
                TraceLog(LOG_INFO,
                         "MapLoader: LIGHT object %s has modelName %s - treating as MODEL (map "
                         "editor export issue)",
                         objectData.name.c_str(), objectData.modelName.c_str());

                // Change type to MODEL and load the model
                objectData.type = MapObjectType::MODEL;
                std::vector<std::string> possiblePaths = ResolveModelPaths(objectData.modelName);
                LoadModelWithErrorHandling(objectData.modelName, possiblePaths, map.m_loadedModels);
            }
            // Also handle LIGHT objects that might have been exported without modelName but should
            // be models
            else if (objectData.type == MapObjectType::LIGHT)
            {
                // Check if this LIGHT object has properties that suggest it should be a MODEL
                // This handles cases where the map editor incorrectly exports models as lights
                bool shouldBeModel = false;

                // Check for model-like properties (non-zero scale, specific naming patterns, etc.)
                if (objectData.scale.x != 1.0f || objectData.scale.y != 1.0f ||
                    objectData.scale.z != 1.0f)
                {
                    shouldBeModel = true;
                }
                else if (objectData.name.find("model") != std::string::npos ||
                         objectData.name.find("Model") != std::string::npos ||
                         objectData.name.find("MODEL") != std::string::npos)
                {
                    shouldBeModel = true;
                }

                if (shouldBeModel)
                {
                    TraceLog(LOG_INFO,
                             "MapLoader: LIGHT object %s appears to be a misclassified MODEL - "
                             "converting",
                             objectData.name.c_str());
                    objectData.type = MapObjectType::MODEL;
                    // Try to infer model name from object name
                    if (objectData.modelName.empty())
                    {
                        objectData.modelName = objectData.name;
                        // Remove common prefixes/suffixes
                        if (objectData.modelName.find("parkour_element_") == 0)
                        {
                            objectData.modelName = objectData.modelName.substr(16);
                        }
                    }
                    std::vector<std::string> possiblePaths =
                        ResolveModelPaths(objectData.modelName);
                    LoadModelWithErrorHandling(objectData.modelName, possiblePaths,
                                               map.m_loadedModels);
                }
            }
        }
    }

    TraceLog(LOG_INFO, "Successfully loaded editor format map: %s with %d objects", path.c_str(),
             map.m_objects.size());

    return map;
}

bool SaveGameMap(const GameMap &map, const std::string &path)
{
    json j;

    // Save metadata
    json metadata;
    metadata["name"] = map.m_metadata.name;
    metadata["displayName"] = map.m_metadata.displayName;
    metadata["description"] = map.m_metadata.description;
    metadata["author"] = map.m_metadata.author;
    metadata["version"] = map.m_metadata.version;
    metadata["difficulty"] = map.m_metadata.difficulty;

    // Save colors
    metadata["skyColor"] = {{"r", map.m_metadata.skyColor.r},
                            {"g", map.m_metadata.skyColor.g},
                            {"b", map.m_metadata.skyColor.b},
                            {"a", map.m_metadata.skyColor.a}};

    metadata["groundColor"] = {{"r", map.m_metadata.groundColor.r},
                               {"g", map.m_metadata.groundColor.g},
                               {"b", map.m_metadata.groundColor.b},
                               {"a", map.m_metadata.groundColor.a}};

    // Save positions
    metadata["startPosition"] = {{"x", map.m_metadata.startPosition.x},
                                 {"y", map.m_metadata.startPosition.y},
                                 {"z", map.m_metadata.startPosition.z}};

    metadata["endPosition"] = {{"x", map.m_metadata.endPosition.x},
                               {"y", map.m_metadata.endPosition.y},
                               {"z", map.m_metadata.endPosition.z}};

    j["metadata"] = metadata;

    // Save objects
    json objects = json::array();
    for (const auto &obj : map.m_objects)
    {
        json object;

        object["name"] = obj.name;
        object["type"] = static_cast<int>(obj.type);

        // Position
        object["position"] = {{"x", obj.position.x}, {"y", obj.position.y}, {"z", obj.position.z}};

        // Rotation
        object["rotation"] = {{"x", obj.rotation.x}, {"y", obj.rotation.y}, {"z", obj.rotation.z}};

        // Scale
        object["scale"] = {{"x", obj.scale.x}, {"y", obj.scale.y}, {"z", obj.scale.z}};

        // Color
        object["color"] = {
            {"r", obj.color.r}, {"g", obj.color.g}, {"b", obj.color.b}, {"a", obj.color.a}};

        // Model name (for MODEL type)
        if (!obj.modelName.empty())
            object["modelName"] = obj.modelName;

        // Shape-specific properties
        object["radius"] = obj.radius;
        object["height"] = obj.height;

        object["size"] = {{"width", obj.size.x}, {"height", obj.size.y}};

        // Collision properties
        object["isPlatform"] = obj.isPlatform;
        object["isObstacle"] = obj.isObstacle;

        objects.push_back(object);
    }

    j["objects"] = objects;

    // Write to file
    std::ofstream file(path);
    if (!file.is_open())
    {
        TraceLog(LOG_ERROR, "Failed to create map file: %s", path.c_str());
        return false;
    }

    file << j.dump(4); // Pretty print with 4 spaces indentation
    TraceLog(LOG_INFO, "Successfully saved map: %s", path.c_str());

    return true;
}

MapObjectData CreateMapObjectFromType(MapObjectType type, const Vector3 &position,
                                      const Vector3 &scale, const Color &color)
{
    MapObjectData obj;
    obj.type = type;
    obj.position = position;
    obj.scale = scale;
    obj.color = color;
    obj.name = "object_" + std::to_string(rand());

    // Ensure consistent scale handling across all object types
    // Normalize scale values to prevent inconsistencies
    obj.scale.x = (scale.x <= 0.0f) ? 1.0f : scale.x;
    obj.scale.y = (scale.y <= 0.0f) ? 1.0f : scale.y;
    obj.scale.z = (scale.z <= 0.0f) ? 1.0f : scale.z;

    switch (type)
    {
    case MapObjectType::SPHERE:
        obj.radius = obj.scale.x; // Use normalized scale.x as radius
        break;
    case MapObjectType::CYLINDER:
        obj.radius = obj.scale.x;
        obj.height = obj.scale.y;
        break;
    case MapObjectType::PLANE:
        obj.size = Vector2{obj.scale.x, obj.scale.z};
        break;
    case MapObjectType::MODEL:
        // For models, scale is used directly in rendering, no additional properties needed
        break;
    case MapObjectType::SPAWN_ZONE:
        // Spawn zone uses default scale, no additional properties needed
        break;
    default:
        break;
    }

    return obj;
}

// ============================================================================
// Skybox Loading Functions
// ============================================================================

namespace
{
// Shader paths for skybox
constexpr const char *SKYBOX_SHADER_VS_FORMAT =
    PROJECT_ROOT_DIR "/resources/shaders/glsl%i/skybox.vs";
constexpr const char *SKYBOX_SHADER_FS_FORMAT =
    PROJECT_ROOT_DIR "/resources/shaders/glsl%i/skybox.fs";
constexpr const char *SKYBOX_CUBEMAP_VS_FORMAT =
    PROJECT_ROOT_DIR "/resources/shaders/glsl%i/cubemap.vs";
constexpr const char *SKYBOX_CUBEMAP_FS_FORMAT =
    PROJECT_ROOT_DIR "/resources/shaders/glsl%i/cubemap.fs";

// Fallback paths
constexpr const char *SKYBOX_SHADER_VS_FORMAT_FALLBACK =
    PROJECT_ROOT_DIR "/include/SkyboxLib/shader/glsl%i/skybox.vs";
constexpr const char *SKYBOX_SHADER_FS_FORMAT_FALLBACK =
    PROJECT_ROOT_DIR "/include/SkyboxLib/shader/glsl%i/skybox.fs";
constexpr const char *SKYBOX_CUBEMAP_VS_FORMAT_FALLBACK =
    PROJECT_ROOT_DIR "/include/SkyboxLib/shader/glsl%i/cubemap.vs";
constexpr const char *SKYBOX_CUBEMAP_FS_FORMAT_FALLBACK =
    PROJECT_ROOT_DIR "/include/SkyboxLib/shader/glsl%i/cubemap.fs";

std::string ResolveSkyboxAbsolutePath(const std::string &texturePath)
{
    if (texturePath.empty())
    {
        return "";
    }

    std::filesystem::path input(texturePath);
    std::error_code ec;

    if (input.is_absolute())
    {
        std::filesystem::path canonical = std::filesystem::weakly_canonical(input, ec);
        if (!ec && std::filesystem::exists(canonical))
        {
            return canonical.string();
        }
        if (std::filesystem::exists(input))
        {
            return input.string();
        }
    }

    std::filesystem::path projectRoot(PROJECT_ROOT_DIR);
    std::filesystem::path combined = projectRoot / input;
    std::filesystem::path canonicalCombined = std::filesystem::weakly_canonical(combined, ec);
    if (!ec && std::filesystem::exists(canonicalCombined))
    {
        return canonicalCombined.string();
    }
    if (std::filesystem::exists(combined))
    {
        return combined.string();
    }

    return (input.is_absolute() ? input : combined).string();
}
} // namespace

void LoadSkyboxForMap(GameMap &map)
{
    if (map.m_metadata.skyboxTexture.empty())
    {
        return;
    }

    std::string absolutePath = ResolveSkyboxAbsolutePath(map.m_metadata.skyboxTexture);
    if (absolutePath.empty() || !std::filesystem::exists(absolutePath))
    {
        TraceLog(LOG_WARNING, "LoadSkyboxForMap() - Skybox texture not found: %s",
                 map.m_metadata.skyboxTexture.c_str());
        return;
    }

    if (!map.m_skybox)
    {
        map.m_skybox = std::make_shared<Skybox>();
    }
    
    map.m_skybox->LoadMaterialTexture(absolutePath);
    TraceLog(LOG_INFO, "LoadSkyboxForMap() - Loaded skybox from %s", absolutePath.c_str());
}

// ============================================================================
// Map Rendering Functions
// ============================================================================

void RenderGameMap(const GameMap &map, Camera3D camera)
{
    // Set sky color if specified (or use default if skybox is present)
    if (map.m_metadata.skyColor.a > 0)
    {
        ClearBackground(map.m_metadata.skyColor);
    }
    else if (!map.m_skybox)
    {
        // Use default sky color if no skybox
        ClearBackground(SKYBLUE);
    }

    BeginMode3D(camera);

    // Render skybox first (if present)
    if (map.m_skybox && map.m_skybox->IsLoaded())
    {
        map.m_skybox->DrawSkybox();
    }

    // Render all objects in the map
    for (const auto &object : map.m_objects)
    {
        RenderMapObject(object, map.m_loadedModels, camera);
    }

    EndMode3D();
}

void RenderMapObject(const MapObjectData &object,
                     const std::unordered_map<std::string, Model> &loadedModels,
                     [[maybe_unused]] Camera3D camera, bool useEditorColors)
{
    // Apply object transformations - ensure consistent order for collision/rendering match
    Matrix translation = MatrixTranslate(object.position.x, object.position.y, object.position.z);
    Matrix scale = MatrixScale(object.scale.x, object.scale.y, object.scale.z);
    Vector3 rotationRad = {object.rotation.x * DEG2RAD, object.rotation.y * DEG2RAD,
                           object.rotation.z * DEG2RAD};
    Matrix rotation = MatrixRotateXYZ(rotationRad);

    // Combine transformations: scale -> rotation -> translation (matches DrawModel/ModelLoader)
    Matrix transform = MatrixMultiply(scale, MatrixMultiply(rotation, translation));

    // For primitives, use direct position and scale (DrawCube, DrawSphere don't use transform
    // matrix) Rotation is not applied to primitives - same as Editor and Game::RenderEditorMap()
    // For models, use transform matrix (DrawModel uses model.transform)
    switch (object.type)
    {
    case MapObjectType::CUBE:
        // DrawCube uses position directly, scale is applied via width/height/length
        DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
        DrawCubeWires(object.position, object.scale.x, object.scale.y, object.scale.z, BLACK);
        break;

    case MapObjectType::SPHERE:
        // DrawSphere uses position directly, radius is from object.radius
        DrawSphere(object.position, object.radius, object.color);
        DrawSphereWires(object.position, object.radius, 16, 16, BLACK);
        break;

    case MapObjectType::CYLINDER:
        // DrawCylinder uses position directly, radius and height from object properties
        DrawCylinder(object.position, object.radius, object.radius, object.height, 16,
                     object.color);
        DrawCylinderWires(object.position, object.radius, object.radius, object.height, 16, BLACK);
        break;

    case MapObjectType::PLANE:
        // DrawPlane uses position directly, size from object.size
        DrawPlane(object.position, Vector2{object.size.x, object.size.y}, object.color);
        break;

    case MapObjectType::SPAWN_ZONE:
        // Spawn zone is rendered separately in game (via MapManager::RenderSpawnZone)
        // Don't render it here as a regular object
        break;

    case MapObjectType::MODEL:
        // Find the corresponding loaded model for this object
        if (!object.modelName.empty())
        {
            // Normalize the lookup key to match how models are stored
            std::string lookupKey = object.modelName;
            std::replace(lookupKey.begin(), lookupKey.end(), '\\', '/');
            std::filesystem::path keyPath(lookupKey);
            std::string keyStem = keyPath.stem().string();
            std::string keyExtension = keyPath.extension().string();
            std::string cleanKey = keyStem;
            if (!keyExtension.empty())
            {
                cleanKey += keyExtension;
            }

            auto it = loadedModels.find(cleanKey);
            if (it != loadedModels.end())
            {
                Model model = it->second;

                // Apply full transform to model (already contains scale, rotation, translation)
                model.transform = transform;

                // Choose tint based on rendering context
                // Editor: use WHITE to preserve textures; Game: use object.color for tinting
                Color tintColor = useEditorColors ? WHITE : object.color;

                // Draw model meshes directly with the full transform matrix
                // This ensures rotation is properly applied (unlike DrawModel which ignores
                // rotation)
                for (int i = 0; i < model.meshCount; i++)
                {
                    // Get material color and apply tint
                    Color color =
                        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color;
                    Color colorTint = WHITE;
                    colorTint.r = (unsigned char)(((int)color.r * (int)tintColor.r) / 255);
                    colorTint.g = (unsigned char)(((int)color.g * (int)tintColor.g) / 255);
                    colorTint.b = (unsigned char)(((int)color.b * (int)tintColor.b) / 255);
                    colorTint.a = (unsigned char)(((int)color.a * (int)tintColor.a) / 255);

                    // Apply tint temporarily
                    model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color =
                        colorTint;

                    // Draw mesh with full transform
                    DrawMesh(model.meshes[i], model.materials[model.meshMaterial[i]], transform);

                    // Restore original color
                    model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = color;
                }

                // Optional: Draw model wires for debugging (disabled in runtime)
                // DrawModelWires(model, Vector3{0, 0, 0}, 1.0f, BLACK);
            }
            else
            {
                // Try fallback lookup with original modelName
                auto it2 = loadedModels.find(object.modelName);
                if (it2 != loadedModels.end())
                {
                    Model model = it2->second;
                    model.transform = transform;

                    // Choose tint based on rendering context
                    Color tintColor = useEditorColors ? WHITE : object.color;

                    // Draw model meshes directly with the full transform matrix
                    for (int i = 0; i < model.meshCount; i++)
                    {
                        // Get material color and apply tint
                        Color color =
                            model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color;
                        Color colorTint = WHITE;
                        colorTint.r = (unsigned char)(((int)color.r * (int)tintColor.r) / 255);
                        colorTint.g = (unsigned char)(((int)color.g * (int)tintColor.g) / 255);
                        colorTint.b = (unsigned char)(((int)color.b * (int)tintColor.b) / 255);
                        colorTint.a = (unsigned char)(((int)color.a * (int)tintColor.a) / 255);

                        // Apply tint temporarily
                        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color =
                            colorTint;

                        // Draw mesh with full transform
                        DrawMesh(model.meshes[i], model.materials[model.meshMaterial[i]],
                                 transform);

                        // Restore original color
                        model.materials[model.meshMaterial[i]].maps[MATERIAL_MAP_DIFFUSE].color = color;
                    }
                    // DrawModelWires(model, Vector3{0, 0, 0}, 1.0f, BLACK);
                }
                else
                {
                    // Model not found, draw placeholder
                    DrawSphere(object.position, 0.5f, RED);
                    TraceLog(LOG_WARNING,
                             "RenderMapObject: Model not found for %s (tried keys: %s, %s)",
                             object.name.c_str(), cleanKey.c_str(), object.modelName.c_str());
                }
            }
        }
        else
        {
            // No model name specified, draw placeholder
            DrawSphere(object.position, 0.5f, RED);
        }
        break;

    case MapObjectType::LIGHT:
        // Light objects don't render visually, they affect lighting
        DrawSphere(object.position, 0.2f, YELLOW); // Visual representation of light
        break;

    default:
        // Unknown object type - draw as cube
        DrawCube(object.position, object.scale.x, object.scale.y, object.scale.z, object.color);
        DrawCubeWires(object.position, object.scale.x, object.scale.y, object.scale.z, BLACK);
        break;
    }
}

// ============================================================================
// Simple MapLoader Implementation
// ============================================================================

GameMap MapLoader::LoadMap(const std::string &path)
{
    return LoadGameMap(path);
}

bool MapLoader::SaveMap(const GameMap &map, const std::string &path)
{
    return SaveGameMap(map, path);
}

std::vector<ModelInfo> MapLoader::LoadModelsFromDirectory(const std::string &directory)
{
    std::vector<ModelInfo> models;
    std::set<std::string> supportedExtensions = {".glb", ".gltf", ".obj", ".fbx", ".dae"};

    try
    {
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
        {
            TraceLog(LOG_WARNING, "Directory does not exist or is not a directory: %s",
                     directory.c_str());
            return models;
        }

        TraceLog(LOG_INFO, "Scanning directory for models: %s", directory.c_str());

        for (const auto &entry : std::filesystem::recursive_directory_iterator(directory))
        {
            if (!entry.is_regular_file())
                continue;

            std::string extension = entry.path().extension().string();
            std::string filename = entry.path().filename().string();

            // Skip hidden files and non-model files
            if (filename.starts_with(".") ||
                supportedExtensions.find(extension) == supportedExtensions.end())
                continue;

            std::string modelPath = entry.path().string();

            // Convert to relative path if within project directory
            std::string projectRoot = PROJECT_ROOT_DIR;
            if (modelPath.find(projectRoot) == 0)
            {
                modelPath = modelPath.substr(projectRoot.length());
                // Ensure path starts with "/" for consistency
                if (!modelPath.empty() && modelPath[0] != '/' && modelPath[0] != '\\')
                {
                    modelPath = "/" + modelPath;
                }
            }

            ModelInfo modelInfo;
            size_t dotPos = filename.find_last_of('.');
            if (dotPos != std::string::npos)
                modelInfo.name = filename.substr(0, dotPos);
            else
                modelInfo.name = filename;
            modelInfo.path = modelPath;
            modelInfo.extension = extension;

            // Determine properties based on file extension and name
            modelInfo.hasAnimations = (extension == ".glb" || extension == ".gltf");
            modelInfo.hasCollision = true;

            models.push_back(modelInfo);

            TraceLog(LOG_INFO, "Found model: %s (%s)", modelInfo.name.c_str(), modelPath.c_str());
        }

        TraceLog(LOG_INFO, "Found %d models in directory: %s", models.size(), directory.c_str());
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Error scanning models directory: %s", e.what());
    }

    return models;
}

bool MapLoader::SaveModelConfig(const std::vector<ModelInfo> &models, const std::string &path)
{
    json j = json::array();

    for (const auto &model : models)
    {
        json modelJson;
        modelJson["name"] = model.name;
        modelJson["path"] = model.path;
        modelJson["spawn"] = true;
        modelJson["hasCollision"] = model.hasCollision;
        modelJson["hasAnimations"] = model.hasAnimations;

        // Add instances array for compatibility
        json instances = json::array();
        json instance;
        instance["position"] = {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}};
        instance["scale"] =
            (model.defaultScale.x + model.defaultScale.y + model.defaultScale.z) / 3.0f;
        instance["spawn"] = true;
        instances.push_back(instance);

        modelJson["instances"] = instances;
        j.push_back(modelJson);
    }

    // Write to file
    std::ofstream file(path);
    if (!file.is_open())
    {
        TraceLog(LOG_ERROR, "Failed to create model config file: %s", path.c_str());
        return false;
    }

    file << j.dump(2);
    TraceLog(LOG_INFO, "Successfully saved model config: %s", path.c_str());

    return true;
}

std::vector<GameMap> MapLoader::LoadAllMapsFromDirectory(const std::string &directory)
{
    std::vector<GameMap> maps;
    std::set<std::string> supportedExtensions = {".json"};

    try
    {
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
        {
            TraceLog(LOG_WARNING, "Directory does not exist or is not a directory: %s",
                     directory.c_str());
            return maps;
        }

        TraceLog(LOG_INFO, "Scanning directory for maps: %s", directory.c_str());

        for (const auto &entry : std::filesystem::directory_iterator(directory))
        {
            if (!entry.is_regular_file())
                continue;

            std::string extension = entry.path().extension().string();
            std::string filename = entry.path().filename().string();

            // Skip hidden files and non-json files
            if (filename.starts_with(".") ||
                supportedExtensions.find(extension) == supportedExtensions.end())
                continue;

            std::string mapPath = entry.path().string();

            // Load the map
            GameMap map = LoadMap(mapPath);
            std::string mapName = map.GetMapMetaData().name;
            if (!map.GetMapObjects().empty() || !mapName.empty())
            {
                maps.push_back(std::move(map));
                TraceLog(LOG_INFO, "Loaded map: %s", mapName.c_str());
            }
        }

        TraceLog(LOG_INFO, "Found %d maps in directory: %s", maps.size(), directory.c_str());
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Error scanning maps directory: %s", e.what());
    }

    return maps;
}

std::vector<std::string> MapLoader::GetMapNamesFromDirectory(const std::string &directory)
{
    std::vector<std::string> names;
    std::set<std::string> supportedExtensions = {".json"};

    try
    {
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
        {
            TraceLog(LOG_WARNING, "Directory does not exist or is not a directory: %s",
                     directory.c_str());
            return names;
        }

        for (const auto &entry : std::filesystem::directory_iterator(directory))
        {
            if (!entry.is_regular_file())
                continue;

            std::string extension = entry.path().extension().string();
            std::string filename = entry.path().filename().string();

            // Skip hidden files and non-json files
            if (filename.starts_with(".") ||
                supportedExtensions.find(extension) == supportedExtensions.end())
                continue;

            // Remove extension to get name
            size_t dotPos = filename.find_last_of('.');
            std::string name;
            if (dotPos != std::string::npos)
                name = filename.substr(0, dotPos);
            else
                name = filename;
            names.push_back(name);
        }
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Error scanning maps directory: %s", e.what());
    }

    return names;
}

Skybox* GameMap::GetSkyBox() const
{
    return m_skybox.get();
}

void GameMap::SetSkyBox(std::shared_ptr<Skybox>& skybox)
{
    m_skybox = skybox;
}

const std::unordered_map<std::string, Model> &GameMap::GetMapModels() const
{
    return m_loadedModels;
}

void GameMap::AddMapModels(const std::unordered_map<std::string, Model> &modelsMap)
{
    m_loadedModels.insert(modelsMap.begin(), modelsMap.end());
}

const std::vector<MapObjectData> &GameMap::GetMapObjects() const
{
    return m_objects;
}

void GameMap::AddMapObjects(const std::vector<MapObjectData> &mapObjects)
{
    m_objects.insert(m_objects.end(), mapObjects.begin(), mapObjects.end());
}

const MapMetadata &GameMap::GetMapMetaData() const
{
    return m_metadata;
}

void GameMap::SetMapMetaData(const MapMetadata &mapData)
{
    m_metadata = mapData;
}
