#include "SceneLoader.h"
#include "core/Log.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
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
            std::string p1 = std::string(PROJECT_ROOT_DIR);
            p1 += "/resources/";
            p1 += normalizedModelName;
            p1 += ext;
            possiblePaths.push_back(p1);

            std::string p2 = std::string(PROJECT_ROOT_DIR);
            p2 += "/resources/models/";
            p2 += normalizedModelName;
            p2 += ext;
            possiblePaths.push_back(p2);
        }

        // Try with stem variations
        for (const auto &ext : extensions)
        {
            std::string p1 = std::string(PROJECT_ROOT_DIR);
            p1 += "/resources/";
            p1 += stem;
            p1 += ext;
            possiblePaths.push_back(p1);

            std::string p2 = std::string(PROJECT_ROOT_DIR);
            p2 += "/resources/models/";
            p2 += stem;
            p2 += ext;
            possiblePaths.push_back(p2);
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
        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" +
                                normalizedModelName);
        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/models/" +
                                normalizedModelName);

        // Try with stem variation
        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/" + stem + extension);
        possiblePaths.push_back(std::string(PROJECT_ROOT_DIR) + "/resources/models/" + stem +
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
                    CD_CORE_INFO("SceneLoader: Successfully loaded model %s (key: %s) from %s "
                                 "(meshCount: %d)",
                                 modelName.c_str(), cleanKey.c_str(), modelPath.c_str(),
                                 model.meshCount);
                    return true;
                }
                else
                {
                    CD_CORE_WARN("SceneLoader: Model loaded but has no meshes: %s",
                                 modelPath.c_str());
                }
            }
            else
            {
                CD_CORE_INFO("SceneLoader: Model %s (key: %s) already loaded", modelName.c_str(),
                             cleanKey.c_str());
                return true;
            }
        }
    }

    CD_CORE_WARN("SceneLoader: Could not find model file for %s. Tried paths:", modelName.c_str());
    for (const auto &path : possiblePaths)
    {
        CD_CORE_WARN("  - %s", path.c_str());
    }
    return false;
}

// GameScene struct implementation
void GameScene::Cleanup()
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

GameScene::~GameScene()
{
    Cleanup();
}

// ============================================================================
// SceneLoader Implementation
// ============================================================================

bool SceneLoader::SaveSceneToFile(const GameScene &map, const std::string &path)
{
    json j;
    const MapMetadata &metadata = map.GetMapMetaData();

    // Save metadata
    json metaJson;
    metaJson["name"] = metadata.name;
    metaJson["displayName"] = metadata.displayName;
    metaJson["description"] = metadata.description;
    metaJson["author"] = metadata.author;
    metaJson["version"] = metadata.version;
    metaJson["difficulty"] = metadata.difficulty;
    metaJson["sceneType"] = static_cast<int>(metadata.sceneType);

    // Save colors - save skyColor if skybox is empty, otherwise save skyboxTexture
    if (metadata.skyboxTexture.empty())
    {
        metaJson["skyColor"] = {{"r", metadata.skyColor.r},
                                {"g", metadata.skyColor.g},
                                {"b", metadata.skyColor.b},
                                {"a", metadata.skyColor.a}};
    }
    else
    {
        metaJson["skyboxTexture"] = metadata.skyboxTexture;
    }

    metaJson["groundColor"] = {{"r", metadata.groundColor.r},
                               {"g", metadata.groundColor.g},
                               {"b", metadata.groundColor.b},
                               {"a", metadata.groundColor.a}};

    // Save positions
    metaJson["startPosition"] = {{"x", metadata.startPosition.x},
                                 {"y", metadata.startPosition.y},
                                 {"z", metadata.startPosition.z}};

    metaJson["endPosition"] = {{"x", metadata.endPosition.x},
                               {"y", metadata.endPosition.y},
                               {"z", metadata.endPosition.z}};

    j["metadata"] = metaJson;

    // Save objects
    json objects = json::array();
    for (const auto &obj : map.GetMapObjects())
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

    // Save UI elements
    json uiElements = json::array();
    for (const auto &elem : map.GetUIElements())
    {
        json uiElem;
        uiElem["name"] = elem.name;
        uiElem["type"] = elem.type;

        // RectTransform
        uiElem["anchor"] = elem.anchor;
        uiElem["position"] = {{"x", elem.position.x}, {"y", elem.position.y}};
        uiElem["size"] = {{"x", elem.size.x}, {"y", elem.size.y}};
        uiElem["pivot"] = {{"x", elem.pivot.x}, {"y", elem.pivot.y}};
        uiElem["rotation"] = elem.rotation;

        // Component-specific properties
        if (!elem.text.empty())
            uiElem["text"] = elem.text;
        if (!elem.fontName.empty())
            uiElem["fontName"] = elem.fontName;
        if (elem.fontSize > 0)
            uiElem["fontSize"] = elem.fontSize;
        if (elem.spacing > 0)
            uiElem["spacing"] = elem.spacing;
        uiElem["textColor"] = {{"r", elem.textColor.r},
                               {"g", elem.textColor.g},
                               {"b", elem.textColor.b},
                               {"a", elem.textColor.a}};

        uiElem["normalColor"] = {{"r", elem.normalColor.r},
                                 {"g", elem.normalColor.g},
                                 {"b", elem.normalColor.b},
                                 {"a", elem.normalColor.a}};
        uiElem["hoverColor"] = {{"r", elem.hoverColor.r},
                                {"g", elem.hoverColor.g},
                                {"b", elem.hoverColor.b},
                                {"a", elem.hoverColor.a}};
        uiElem["pressedColor"] = {{"r", elem.pressedColor.r},
                                  {"g", elem.pressedColor.g},
                                  {"b", elem.pressedColor.b},
                                  {"a", elem.pressedColor.a}};

        if (!elem.eventId.empty())
            uiElem["eventId"] = elem.eventId;

        uiElem["borderRadius"] = elem.borderRadius;
        uiElem["borderWidth"] = elem.borderWidth;
        uiElem["borderColor"] = {{"r", elem.borderColor.r},
                                 {"g", elem.borderColor.g},
                                 {"b", elem.borderColor.b},
                                 {"a", elem.borderColor.a}};

        uiElem["tint"] = {
            {"r", elem.tint.r}, {"g", elem.tint.g}, {"b", elem.tint.b}, {"a", elem.tint.a}};

        if (!elem.texturePath.empty())
            uiElem["texturePath"] = elem.texturePath;

        // Action System
        if (!elem.actionType.empty())
            uiElem["actionType"] = elem.actionType;
        if (!elem.actionTarget.empty())
            uiElem["actionTarget"] = elem.actionTarget;

        uiElements.push_back(uiElem);
    }

    j["uiElements"] = uiElements;

    // Write to file
    std::ofstream file(path);
    if (!file.is_open())
    {
        CD_CORE_ERROR("Failed to create map file: %s", path.c_str());
        return false;
    }

    file << j.dump(4); // Pretty print with 4 spaces indentation
    CD_CORE_INFO("Successfully saved map: %s", path.c_str());

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
        obj.radius = obj.scale.x * 0.5f; // Use normalized scale.x as diameter
        break;
    case MapObjectType::CYLINDER:
        obj.radius = obj.scale.x * 0.5f;
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
    PROJECT_ROOT_DIR "/include/skyboxLib/shader/glsl%i/skybox.vs";
constexpr const char *SKYBOX_SHADER_FS_FORMAT_FALLBACK =
    PROJECT_ROOT_DIR "/include/skyboxLib/shader/glsl%i/skybox.fs";
constexpr const char *SKYBOX_CUBEMAP_VS_FORMAT_FALLBACK =
    PROJECT_ROOT_DIR "/include/skyboxLib/shader/glsl%i/cubemap.vs";
constexpr const char *SKYBOX_CUBEMAP_FS_FORMAT_FALLBACK =
    PROJECT_ROOT_DIR "/include/skyboxLib/shader/glsl%i/cubemap.fs";

std::string ResolveSkyboxAbsolutePath(const std::string &texturePath)
{
    if (texturePath.empty())
    {
        return "";
    }

    // 1. Check if path is already absolute and exists
    std::filesystem::path input(texturePath);
    if (input.is_absolute() && std::filesystem::exists(input))
    {
        return input.string();
    }

    // 2. Check relative to current working directory
    if (std::filesystem::exists(input))
    {
        return std::filesystem::absolute(input).string();
    }

    std::filesystem::path projectRoot(PROJECT_ROOT_DIR);

    // 3. Check relative to Project Root
    std::filesystem::path combined = projectRoot / input;
    if (std::filesystem::exists(combined))
    {
        return combined.string();
    }

    // 4. Check in resources folder if usage didn't specify it
    if (texturePath.find("resources") == std::string::npos)
    {
        std::filesystem::path resourcesPath = projectRoot / "resources" / input;
        if (std::filesystem::exists(resourcesPath))
        {
            return resourcesPath.string();
        }

        // 5. Try in skyboxes folder specifically
        std::filesystem::path skyboxPath = projectRoot / "resources" / "skyboxes" / input;
        if (std::filesystem::exists(skyboxPath))
        {
            return skyboxPath.string();
        }
    }

    // Return original if nothing found (will fail later with clearer error)
    return (input.is_absolute() ? input : combined).string();
}
} // namespace

void SceneLoader::LoadSkyboxForScene(GameScene &map)
{
    const MapMetadata &metadata = map.GetMapMetaData();
    if (metadata.skyboxTexture.empty())
    {
        return;
    }

    std::string absolutePath = ResolveSkyboxAbsolutePath(metadata.skyboxTexture);
    if (absolutePath.empty() || !std::filesystem::exists(absolutePath))
    {
        CD_CORE_WARN("LoadSkyboxForScene() - Skybox texture not found: %s",
                     metadata.skyboxTexture.c_str());
        return;
    }

    if (!map.GetSkyBox())
    {
        std::shared_ptr<Skybox> skybox = std::make_shared<Skybox>();
        // Init() automatically loads shaders, so shaders are ready before loading texture
        skybox->Init();
        map.SetSkyBox(skybox);
    }

    Skybox *skybox = map.GetSkyBox();
    if (skybox)
    {
        skybox->LoadMaterialTexture(absolutePath);
        CD_CORE_INFO("LoadSkyboxForScene() - Loaded skybox from %s", absolutePath.c_str());
    }
}

// ============================================================================
// SceneLoader Public Methods
// ============================================================================

GameScene SceneLoader::LoadScene(const std::string &path)
{
    GameScene map;

    std::ifstream file(path);
    if (!file.is_open())
    {
        CD_CORE_ERROR("Failed to open map file: %s", path.c_str());
        return map;
    }
    json j;
    try
    {
        file >> j;
    }
    catch (const std::exception &e)
    {
        CD_CORE_ERROR("Failed to parse map JSON: %s", e.what());
        return map;
    }
    // Load metadata
    if (j.contains("metadata"))
    {
        const auto &meta = j["metadata"];
        // By default, set some reasonable defaults
        MapMetadata &metadata = map.GetMapMetaDataMutable();
        metadata.name = meta.value("name", "unnamed_map");
        metadata.displayName = meta.value("displayName", "Unnamed Map");
        metadata.description = meta.value("description", "");
        metadata.author = meta.value("author", "");
        metadata.version = meta.value("version", "1.0");

        metadata.difficulty = meta.value("difficulty", 1.0f);
        metadata.sceneType =
            static_cast<SceneType>(meta.value("sceneType", static_cast<int>(SceneType::LEVEL_3D)));

        // Load colors
        if (meta.contains("skyColor"))
        {
            auto &sky = meta["skyColor"];
            metadata.skyColor = Color{static_cast<unsigned char>(sky.value("r", 135)),
                                      static_cast<unsigned char>(sky.value("g", 206)),
                                      static_cast<unsigned char>(sky.value("b", 235)),
                                      static_cast<unsigned char>(sky.value("a", 255))};
        }

        if (meta.contains("groundColor"))
        {
            auto &ground = meta["groundColor"];
            metadata.groundColor = Color{static_cast<unsigned char>(ground.value("r", 34)),
                                         static_cast<unsigned char>(ground.value("g", 139)),
                                         static_cast<unsigned char>(ground.value("b", 34)),
                                         static_cast<unsigned char>(ground.value("a", 255))};
        }

        // Load positions
        if (meta.contains("startPosition"))
        {
            auto &start = meta["startPosition"];
            metadata.startPosition =
                Vector3{start.value("x", 0.0f), start.value("y", 0.0f), start.value("z", 0.0f)};
        }

        if (meta.contains("endPosition"))
        {
            auto &end = meta["endPosition"];
            metadata.endPosition =
                Vector3{end.value("x", 0.0f), end.value("y", 0.0f), end.value("z", 0.0f)};
        }

        // Load skybox texture path
        if (meta.contains("skyboxTexture"))
        {
            metadata.skyboxTexture = meta.value("skyboxTexture", "");
        }
    }

    // Note: Skybox loading will be done by the caller after map is loaded
    // This allows the caller to control when skybox is loaded

    // Load objects
    if (j.contains("objects"))
    {
        size_t objectIndex = map.GetMapObjects().size();
        for (const auto &obj : j["objects"])
        {
            MapObjectData objectData;

            // Basic properties
            objectData.name = obj.value("name", "object_" + std::to_string(objectIndex++));
            objectData.type = static_cast<MapObjectType>(obj.value("type", 0));
            CD_CORE_INFO("SceneLoader: Loading object %s, type %d", objectData.name.c_str(),
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

            map.GetMapObjectsMutable().push_back(objectData);

            // Load model if it's a MODEL type object
            if (objectData.type == MapObjectType::MODEL && !objectData.modelName.empty())
            {
                CD_CORE_INFO("SceneLoader: Loading MODEL object %s with modelName %s",
                             objectData.name.c_str(), objectData.modelName.c_str());

                // Use helper function to resolve paths and load model
                std::vector<std::string> possiblePaths = ResolveModelPaths(objectData.modelName);
                LoadModelWithErrorHandling(objectData.modelName, possiblePaths,
                                           map.GetMapModelsMutable());
            }
            // Handle LIGHT type objects that may actually be misclassified MODEL objects from map
            // editor
            else if (objectData.type == MapObjectType::LIGHT && !objectData.modelName.empty())
            {
                CD_CORE_INFO(
                    "SceneLoader: LIGHT object %s has modelName %s - treating as MODEL (map "
                    "editor export issue)",
                    objectData.name.c_str(), objectData.modelName.c_str());

                // Change type to MODEL and load the model
                objectData.type = MapObjectType::MODEL;
                std::vector<std::string> possiblePaths = ResolveModelPaths(objectData.modelName);
                LoadModelWithErrorHandling(objectData.modelName, possiblePaths,
                                           map.GetMapModelsMutable());
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
                    CD_CORE_INFO(
                        "SceneLoader: LIGHT object %s appears to be a misclassified MODEL - "
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
                                               map.GetMapModelsMutable());
                }
            }
        }
    }

    CD_CORE_INFO("Successfully loaded editor format map: %s with %d objects", path.c_str(),
                 map.GetMapObjects().size());

    // Load UI elements if present
    if (j.contains("uiElements") && j["uiElements"].is_array())
    {
        for (const auto &uiElem : j["uiElements"])
        {
            UIElementData elemData;

            elemData.name = uiElem.value("name", "Unnamed UI");
            elemData.type = uiElem.value("type", "");

            // RectTransform
            elemData.anchor = uiElem.value("anchor", 0);
            if (uiElem.contains("position"))
            {
                auto &pos = uiElem["position"];
                elemData.position = Vector2{pos.value("x", 0.0f), pos.value("y", 0.0f)};
            }
            if (uiElem.contains("size"))
            {
                auto &sz = uiElem["size"];
                elemData.size = Vector2{sz.value("x", 100.0f), sz.value("y", 40.0f)};
            }
            if (uiElem.contains("pivot"))
            {
                auto &piv = uiElem["pivot"];
                elemData.pivot = Vector2{piv.value("x", 0.5f), piv.value("y", 0.5f)};
            }
            elemData.rotation = uiElem.value("rotation", 0.0f);

            // Component-specific properties
            elemData.text = uiElem.value("text", "");
            elemData.fontName = uiElem.value("fontName", "");
            elemData.fontSize = uiElem.value("fontSize", 20);
            elemData.spacing = uiElem.value("spacing", 1.0f);

            if (uiElem.contains("textColor"))
            {
                auto &col = uiElem["textColor"];
                elemData.textColor = Color{static_cast<unsigned char>(col.value("r", 255)),
                                           static_cast<unsigned char>(col.value("g", 255)),
                                           static_cast<unsigned char>(col.value("b", 255)),
                                           static_cast<unsigned char>(col.value("a", 255))};
            }

            if (uiElem.contains("normalColor"))
            {
                auto &col = uiElem["normalColor"];
                elemData.normalColor = Color{static_cast<unsigned char>(col.value("r", 200)),
                                             static_cast<unsigned char>(col.value("g", 200)),
                                             static_cast<unsigned char>(col.value("b", 200)),
                                             static_cast<unsigned char>(col.value("a", 255))};
            }

            if (uiElem.contains("hoverColor"))
            {
                auto &col = uiElem["hoverColor"];
                elemData.hoverColor = Color{static_cast<unsigned char>(col.value("r", 220)),
                                            static_cast<unsigned char>(col.value("g", 220)),
                                            static_cast<unsigned char>(col.value("b", 220)),
                                            static_cast<unsigned char>(col.value("a", 255))};
            }

            if (uiElem.contains("pressedColor"))
            {
                auto &col = uiElem["pressedColor"];
                elemData.pressedColor = Color{static_cast<unsigned char>(col.value("r", 180)),
                                              static_cast<unsigned char>(col.value("g", 180)),
                                              static_cast<unsigned char>(col.value("b", 180)),
                                              static_cast<unsigned char>(col.value("a", 255))};
            }

            elemData.eventId = uiElem.value("eventId", "");

            elemData.borderRadius = uiElem.value("borderRadius", 0.0f);
            elemData.borderWidth = uiElem.value("borderWidth", 0.0f);
            if (uiElem.contains("borderColor"))
            {
                auto &col = uiElem["borderColor"];
                elemData.borderColor = Color{static_cast<unsigned char>(col.value("r", 0)),
                                             static_cast<unsigned char>(col.value("g", 0)),
                                             static_cast<unsigned char>(col.value("b", 0)),
                                             static_cast<unsigned char>(col.value("a", 255))};
            }

            if (uiElem.contains("tint"))
            {
                auto &col = uiElem["tint"];
                elemData.tint = Color{static_cast<unsigned char>(col.value("r", 255)),
                                      static_cast<unsigned char>(col.value("g", 255)),
                                      static_cast<unsigned char>(col.value("b", 255)),
                                      static_cast<unsigned char>(col.value("a", 255))};
            }

            elemData.texturePath = uiElem.value("texturePath", "");

            // Action System
            elemData.actionType = uiElem.value("actionType", "None");
            elemData.actionTarget = uiElem.value("actionTarget", "");

            map.GetUIElementsMutable().push_back(elemData);
        }

        CD_CORE_INFO("Loaded %d UI elements from map", map.GetUIElements().size());
    }

    // Load skybox if texture path is specified
    if (!map.GetMapMetaData().skyboxTexture.empty())
    {
        LoadSkyboxForScene(map);
    }

    return map;
}

bool SceneLoader::SaveScene(const GameScene &map, const std::string &path)
{
    return SaveSceneToFile(map, path);
}

std::vector<ModelInfo> SceneLoader::LoadModelsFromDirectory(const std::string &directory)
{
    std::vector<ModelInfo> models;
    std::set<std::string> supportedExtensions = {".glb", ".gltf", ".obj", ".fbx", ".dae"};

    try
    {
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
        {
            CD_CORE_WARN("Directory does not exist or is not a directory: %s", directory.c_str());
            return models;
        }

        CD_CORE_INFO("Scanning directory for models: %s", directory.c_str());

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

            CD_CORE_INFO("Found model: %s (%s)", modelInfo.name.c_str(), modelPath.c_str());
        }

        CD_CORE_INFO("Found %d models in directory: %s", models.size(), directory.c_str());
    }
    catch (const std::exception &e)
    {
        CD_CORE_ERROR("Error scanning models directory: %s", e.what());
    }

    return models;
}

bool SceneLoader::SaveModelConfig(const std::vector<ModelInfo> &models, const std::string &path)
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
        CD_CORE_ERROR("Failed to create model config file: %s", path.c_str());
        return false;
    }

    file << j.dump(2);
    CD_CORE_INFO("Successfully saved model config: %s", path.c_str());

    return true;
}

std::vector<GameScene> SceneLoader::LoadAllScenesFromDirectory(const std::string &directory)
{
    std::vector<GameScene> maps;
    std::set<std::string> supportedExtensions = {".json", ".scene"};

    try
    {
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
        {
            CD_CORE_WARN("Directory does not exist or is not a directory: %s", directory.c_str());
            return maps;
        }

        CD_CORE_INFO("Scanning directory for maps: %s", directory.c_str());

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
            GameScene map = LoadScene(mapPath);
            std::string mapName = map.GetMapMetaData().name;
            if (!map.GetMapObjects().empty() || !mapName.empty())
            {
                maps.push_back(std::move(map));
                CD_CORE_INFO("Loaded map: %s", mapName.c_str());
            }
        }

        CD_CORE_INFO("Found %d maps in directory: %s", maps.size(), directory.c_str());
    }
    catch (const std::exception &e)
    {
        CD_CORE_ERROR("Error scanning maps directory: %s", e.what());
    }

    return maps;
}

std::vector<std::string> SceneLoader::GetSceneNamesFromDirectory(const std::string &directory)
{
    std::vector<std::string> names;
    std::set<std::string> supportedExtensions = {".json", ".scene"};

    try
    {
        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
        {
            CD_CORE_WARN("Directory does not exist or is not a directory: %s", directory.c_str());
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
        CD_CORE_ERROR("Error scanning maps directory: %s", e.what());
    }

    return names;
}

Skybox *GameScene::GetSkyBox() const
{
    return m_skybox.get();
}

void GameScene::SetSkyBox(std::shared_ptr<Skybox> &skybox)
{
    m_skybox = skybox;
}

const std::unordered_map<std::string, Model> &GameScene::GetMapModels() const
{
    return m_loadedModels;
}

void GameScene::AddMapModels(const std::unordered_map<std::string, Model> &modelsMap)
{
    m_loadedModels.insert(modelsMap.begin(), modelsMap.end());
}

const std::vector<MapObjectData> &GameScene::GetMapObjects() const
{
    return m_objects;
}

void GameScene::AddMapObjects(const std::vector<MapObjectData> &mapObjects)
{
    m_objects.insert(m_objects.end(), mapObjects.begin(), mapObjects.end());
}

const MapMetadata &GameScene::GetMapMetaData() const
{
    return m_metadata;
}

void GameScene::SetMapMetaData(const MapMetadata &mapData)
{
    m_metadata = mapData;
}

MapMetadata &GameScene::GetMapMetaDataMutable()
{
    return m_metadata;
}

std::unordered_map<std::string, Model> &GameScene::GetMapModelsMutable()
{
    return m_loadedModels;
}

std::vector<MapObjectData> &GameScene::GetMapObjectsMutable()
{
    return m_objects;
}

// UI Elements
const std::vector<UIElementData> &GameScene::GetUIElements() const
{
    return m_uiElements;
}

void GameScene::AddUIElements(const std::vector<UIElementData> &uiElements)
{
    m_uiElements.insert(m_uiElements.end(), uiElements.begin(), uiElements.end());
}

std::vector<UIElementData> &GameScene::GetUIElementsMutable()
{
    return m_uiElements;
}
