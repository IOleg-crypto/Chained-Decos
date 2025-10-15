#include "MapLoader.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <raymath.h>
#include <iostream>
#include <filesystem>
#include <set>
#include <algorithm>

using json = nlohmann::json;

// GameMap struct implementation
void GameMap::Cleanup()
{
    for (auto& model : loadedModels)
    {
        if (model.meshCount > 0)
            UnloadModel(model);
    }
    loadedModels.clear();
}

// ============================================================================
// Legacy MapLoader functions (for backward compatibility)
// ============================================================================

std::vector<MapLoader> LoadMap(const std::string &path)
{
    std::ifstream file(path);
    if (!FileExists(path.c_str()))
    {
        TraceLog(LOG_ERROR, "The map doesn't exist at current path");
        return {};
    }

    json j;
    file >> j;
    std::vector<MapLoader> objects;

    for (const auto &obj : j["objects"])
    {
        LegacyMapLoader mo;
        mo.modelName = obj["model"];
        mo.position = {static_cast<float>(obj["position"][0]),
                       static_cast<float>(obj["position"][1]),
                       static_cast<float>(obj["position"][2])};
        mo.rotation = {static_cast<float>(obj["rotation"][0]),
                       static_cast<float>(obj["rotation"][1]),
                       static_cast<float>(obj["rotation"][2])};
        mo.scale = {static_cast<float>(obj["scale"][0]),
                    static_cast<float>(obj["scale"][1]),
                    static_cast<float>(obj["scale"][2])};

        // Try to load model from different paths
        std::string modelPath = "resources/" + mo.modelName;
        if (FileExists(modelPath.c_str()))
        {
            mo.loadedModel = LoadModel(modelPath.c_str());
        }
        else
        {
            modelPath = "models/" + mo.modelName;
            if (FileExists(modelPath.c_str()))
            {
                mo.loadedModel = LoadModel(modelPath.c_str());
            }
            else
            {
                TraceLog(LOG_WARNING, "Model not found: %s", mo.modelName.c_str());
                continue;
            }
        }

        // Apply transformations
        mo.loadedModel.transform = MatrixScale(mo.scale.x, mo.scale.y, mo.scale.z);

        Matrix rotX = MatrixRotateX(DEG2RAD * mo.rotation.x);
        Matrix rotY = MatrixRotateY(DEG2RAD * mo.rotation.y);
        Matrix rotZ = MatrixRotateZ(DEG2RAD * mo.rotation.z);
        mo.loadedModel.transform = MatrixMultiply(mo.loadedModel.transform, rotX);
        mo.loadedModel.transform = MatrixMultiply(mo.loadedModel.transform, rotY);
        mo.loadedModel.transform = MatrixMultiply(mo.loadedModel.transform, rotZ);

        mo.loadedModel.transform = MatrixMultiply(
            mo.loadedModel.transform, MatrixTranslate(mo.position.x, mo.position.y, mo.position.z));

        // objects.push_back(mo); // Legacy objects are handled differently now
    }

    return objects;
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
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Failed to parse map JSON: %s", e.what());
        return map;
    }

    // Load metadata
    if (j.contains("metadata"))
    {
        const auto& meta = j["metadata"];
        map.metadata.name = meta.value("name", "unnamed_map");
        map.metadata.displayName = meta.value("displayName", "Unnamed Map");
        map.metadata.description = meta.value("description", "");
        map.metadata.author = meta.value("author", "");
        map.metadata.version = meta.value("version", "1.0");
        map.metadata.difficulty = meta.value("difficulty", 1.0f);

        // Load colors
        if (meta.contains("skyColor"))
        {
            auto& sky = meta["skyColor"];
            map.metadata.skyColor = Color{
                static_cast<unsigned char>(sky.value("r", 135)),
                static_cast<unsigned char>(sky.value("g", 206)),
                static_cast<unsigned char>(sky.value("b", 235)),
                static_cast<unsigned char>(sky.value("a", 255))
            };
        }

        if (meta.contains("groundColor"))
        {
            auto& ground = meta["groundColor"];
            map.metadata.groundColor = Color{
                static_cast<unsigned char>(ground.value("r", 34)),
                static_cast<unsigned char>(ground.value("g", 139)),
                static_cast<unsigned char>(ground.value("b", 34)),
                static_cast<unsigned char>(ground.value("a", 255))
            };
        }

        // Load positions
        if (meta.contains("startPosition"))
        {
            auto& start = meta["startPosition"];
            map.metadata.startPosition = Vector3{
                start.value("x", 0.0f),
                start.value("y", 0.0f),
                start.value("z", 0.0f)
            };
        }

        if (meta.contains("endPosition"))
        {
            auto& end = meta["endPosition"];
            map.metadata.endPosition = Vector3{
                end.value("x", 0.0f),
                end.value("y", 0.0f),
                end.value("z", 0.0f)
            };
        }
    }

    // Load objects
    if (j.contains("objects"))
    {
        for (const auto& obj : j["objects"])
        {
            MapObjectData objectData;

            // Basic properties
            objectData.name = obj.value("name", "object_" + std::to_string(map.objects.size()));
            objectData.type = static_cast<MapObjectType>(obj.value("type", 0));

            // Position
            if (obj.contains("position"))
            {
                auto& pos = obj["position"];
                objectData.position = Vector3{
                    pos.value("x", 0.0f),
                    pos.value("y", 0.0f),
                    pos.value("z", 0.0f)
                };
            }

            // Rotation
            if (obj.contains("rotation"))
            {
                auto& rot = obj["rotation"];
                objectData.rotation = Vector3{
                    rot.value("x", 0.0f),
                    rot.value("y", 0.0f),
                    rot.value("z", 0.0f)
                };
            }

            // Scale
            if (obj.contains("scale"))
            {
                auto& scl = obj["scale"];
                objectData.scale = Vector3{
                    scl.value("x", 1.0f),
                    scl.value("y", 1.0f),
                    scl.value("z", 1.0f)
                };
            }

            // Color
            if (obj.contains("color"))
            {
                auto& col = obj["color"];
                objectData.color = Color{
                    static_cast<unsigned char>(col.value("r", 255)),
                    static_cast<unsigned char>(col.value("g", 255)),
                    static_cast<unsigned char>(col.value("b", 255)),
                    static_cast<unsigned char>(col.value("a", 255))
                };
            }

            // Model name (for MODEL type)
            objectData.modelName = obj.value("modelName", "");

            // Shape-specific properties
            objectData.radius = obj.value("radius", 1.0f);
            objectData.height = obj.value("height", 1.0f);

            if (obj.contains("size"))
            {
                auto& sz = obj["size"];
                objectData.size = Vector2{
                    sz.value("width", 1.0f),
                    sz.value("height", 1.0f)
                };
            }

            map.objects.push_back(objectData);

            // Load model if it's a MODEL type object
            if (objectData.type == MapObjectType::MODEL && !objectData.modelName.empty())
            {
                std::string modelPath = "resources/" + objectData.modelName;
                if (FileExists(modelPath.c_str()))
                {
                    Model model = LoadModel(modelPath.c_str());
                    map.loadedModels.push_back(model);
                }
                else
                {
                    TraceLog(LOG_WARNING, "Model not found: %s", objectData.modelName.c_str());
                }
            }
        }
    }

    TraceLog(LOG_INFO, "Successfully loaded map: %s with %d objects",
             map.metadata.name.c_str(), map.objects.size());

    return map;
}

bool SaveGameMap(const GameMap& map, const std::string& path)
{
    json j;

    // Save metadata
    json metadata;
    metadata["name"] = map.metadata.name;
    metadata["displayName"] = map.metadata.displayName;
    metadata["description"] = map.metadata.description;
    metadata["author"] = map.metadata.author;
    metadata["version"] = map.metadata.version;
    metadata["difficulty"] = map.metadata.difficulty;

    // Save colors
    metadata["skyColor"] = {
        {"r", map.metadata.skyColor.r},
        {"g", map.metadata.skyColor.g},
        {"b", map.metadata.skyColor.b},
        {"a", map.metadata.skyColor.a}
    };

    metadata["groundColor"] = {
        {"r", map.metadata.groundColor.r},
        {"g", map.metadata.groundColor.g},
        {"b", map.metadata.groundColor.b},
        {"a", map.metadata.groundColor.a}
    };

    // Save positions
    metadata["startPosition"] = {
        {"x", map.metadata.startPosition.x},
        {"y", map.metadata.startPosition.y},
        {"z", map.metadata.startPosition.z}
    };

    metadata["endPosition"] = {
        {"x", map.metadata.endPosition.x},
        {"y", map.metadata.endPosition.y},
        {"z", map.metadata.endPosition.z}
    };

    j["metadata"] = metadata;

    // Save objects
    json objects = json::array();
    for (const auto& obj : map.objects)
    {
        json object;

        object["name"] = obj.name;
        object["type"] = static_cast<int>(obj.type);

        // Position
        object["position"] = {
            {"x", obj.position.x},
            {"y", obj.position.y},
            {"z", obj.position.z}
        };

        // Rotation
        object["rotation"] = {
            {"x", obj.rotation.x},
            {"y", obj.rotation.y},
            {"z", obj.rotation.z}
        };

        // Scale
        object["scale"] = {
            {"x", obj.scale.x},
            {"y", obj.scale.y},
            {"z", obj.scale.z}
        };

        // Color
        object["color"] = {
            {"r", obj.color.r},
            {"g", obj.color.g},
            {"b", obj.color.b},
            {"a", obj.color.a}
        };

        // Model name (for MODEL type)
        if (!obj.modelName.empty())
            object["modelName"] = obj.modelName;

        // Shape-specific properties
        object["radius"] = obj.radius;
        object["height"] = obj.height;

        object["size"] = {
            {"width", obj.size.x},
            {"height", obj.size.y}
        };

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

bool ExportMapForEditor(const GameMap& map, const std::string& path)
{
    // Same as SaveGameMap for now, but can be extended for editor-specific features
    return SaveGameMap(map, path);
}

// ============================================================================
// Models.json format map loading (simple array of models with instances)
// ============================================================================

GameMap LoadModelsMap(const std::string& path)
{
    GameMap map;

    if (!FileExists(path.c_str()))
    {
        TraceLog(LOG_ERROR, "Models map file not found: %s", path.c_str());
        return map;
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        TraceLog(LOG_ERROR, "Failed to open models map file: %s", path.c_str());
        return map;
    }

    json j;
    try
    {
        file >> j;
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Failed to parse models map JSON: %s", e.what());
        return map;
    }

    // This format is a simple array of model definitions
    if (j.is_array())
    {
        for (const auto& modelDef : j)
        {
            std::string modelName = modelDef.value("name", "");
            std::string modelPath = modelDef.value("path", "");

            if (modelPath.empty() || modelName.empty())
                continue;

            // Load the model
            std::string fullPath = "resources/" + modelPath;
            if (FileExists(fullPath.c_str()))
            {
                Model model = LoadModel(fullPath.c_str());
                map.loadedModels.push_back(model);

                // Process instances
                if (modelDef.contains("instances") && modelDef["instances"].is_array())
                {
                    for (const auto& instance : modelDef["instances"])
                    {
                        MapObjectData objectData;
                        objectData.name = modelName + "_instance_" + std::to_string(map.objects.size());
                        objectData.type = MapObjectType::MODEL;
                        objectData.modelName = modelPath;

                        // Position
                        if (instance.contains("position"))
                        {
                            auto& pos = instance["position"];
                            objectData.position = Vector3{
                                pos.value("x", 0.0f),
                                pos.value("y", 0.0f),
                                pos.value("z", 0.0f)
                            };
                        }

                        // Scale
                        objectData.scale = Vector3{
                            instance.value("scale", 1.0f),
                            instance.value("scale", 1.0f),
                            instance.value("scale", 1.0f)
                        };

                        // Spawn flag (affects whether this instance should be spawned)
                        bool shouldSpawn = instance.value("spawn", false);

                        map.objects.push_back(objectData);
                    }
                }
            }
            else
            {
                TraceLog(LOG_WARNING, "Model file not found: %s", fullPath.c_str());
            }
        }
    }

    TraceLog(LOG_INFO, "Successfully loaded models map: %s with %d objects",
             path.c_str(), map.objects.size());

    return map;
}

// ============================================================================
// Models.json format map saving (simple array of models with instances)
// ============================================================================

bool SaveModelsMap(const GameMap& map, const std::string& path)
{
    json j = json::array();

    // Group objects by model name to create model definitions with instances
    std::map<std::string, std::vector<const MapObjectData*>> modelGroups;

    for (const auto& object : map.objects)
    {
        if (object.type == MapObjectType::MODEL && !object.modelName.empty())
        {
            modelGroups[object.modelName].push_back(&object);
        }
    }

    // Create model definitions with instances
    for (const auto& [modelPath, objects] : modelGroups)
    {
        if (objects.empty()) continue;

        json modelDef;

        // Get model name from path (remove "resources/" prefix if present)
        std::string modelName = modelPath;
        if (modelName.starts_with("resources/"))
        {
            modelName = modelName.substr(10); // Remove "resources/" prefix
        }

        modelDef["name"] = modelName;
        modelDef["path"] = modelPath;
        modelDef["spawn"] = true; // Default to spawn
        modelDef["hasCollision"] = true; // Default to has collision
        modelDef["collisionPrecision"] = "bvh_only";
        modelDef["hasAnimations"] = false; // Default to no animations

        // Create instances array
        json instances = json::array();
        for (const auto* object : objects)
        {
            json instance;
            instance["position"] = {
                {"x", object->position.x},
                {"y", object->position.y},
                {"z", object->position.z}
            };
            instance["scale"] = (object->scale.x + object->scale.y + object->scale.z) / 3.0f; // Average scale
            instance["spawn"] = true; // Default to spawn

            instances.push_back(instance);
        }

        modelDef["instances"] = instances;
        j.push_back(modelDef);
    }

    // Write to file
    std::ofstream file(path);
    if (!file.is_open())
    {
        TraceLog(LOG_ERROR, "Failed to create models map file: %s", path.c_str());
        return false;
    }

    file << j.dump(2); // Pretty print with 2 spaces indentation
    TraceLog(LOG_INFO, "Successfully saved models map: %s", path.c_str());

    return true;
}

MapObjectData CreateMapObjectFromType(MapObjectType type, const Vector3& position, const Vector3& scale, const Color& color)
{
    MapObjectData obj;
    obj.type = type;
    obj.position = position;
    obj.scale = scale;
    obj.color = color;
    obj.name = "object_" + std::to_string(rand());

    switch (type)
    {
        case MapObjectType::SPHERE:
            obj.radius = scale.x; // Use scale.x as radius
            break;
        case MapObjectType::CYLINDER:
            obj.radius = scale.x;
            obj.height = scale.y;
            break;
        case MapObjectType::PLANE:
            obj.size = Vector2{scale.x, scale.z};
            break;
        default:
            break;
    }

    return obj;
}

// ============================================================================
// Map Rendering Functions
// ============================================================================

void RenderGameMap(const GameMap& map, Camera3D camera)
{
    // Set sky color if specified
    if (map.metadata.skyColor.a > 0)
    {
        ClearBackground(map.metadata.skyColor);
    }

    BeginMode3D(camera);

    // Render all objects in the map
    for (const auto& object : map.objects)
    {
        RenderMapObject(object, map.loadedModels, camera);
    }

    EndMode3D();
}

void RenderMapObject(const MapObjectData& object, const std::vector<Model>& loadedModels, [[maybe_unused]] Camera3D camera)
{
    // Apply object transformations
    Matrix translation = MatrixTranslate(object.position.x, object.position.y, object.position.z);
    Matrix scale = MatrixScale(object.scale.x, object.scale.y, object.scale.z);
    Matrix rotationX = MatrixRotateX(object.rotation.x * DEG2RAD);
    Matrix rotationY = MatrixRotateY(object.rotation.y * DEG2RAD);
    Matrix rotationZ = MatrixRotateZ(object.rotation.z * DEG2RAD);

    // Combine transformations: scale -> rotate -> translate
    Matrix transform = MatrixMultiply(scale, rotationX);
    transform = MatrixMultiply(transform, rotationY);
    transform = MatrixMultiply(transform, rotationZ);
    transform = MatrixMultiply(transform, translation);

    switch (object.type)
    {
        case MapObjectType::CUBE:
            DrawCube(Vector3{0, 0, 0}, 1.0f, 1.0f, 1.0f, object.color);
            DrawCubeWires(Vector3{0, 0, 0}, 1.0f, 1.0f, 1.0f, BLACK);
            break;

        case MapObjectType::SPHERE:
            DrawSphere(Vector3{0, 0, 0}, object.radius, object.color);
            DrawSphereWires(Vector3{0, 0, 0}, object.radius, 16, 16, BLACK);
            break;

        case MapObjectType::CYLINDER:
            DrawCylinder(Vector3{0, 0, 0}, object.radius, object.radius, object.height, 16, object.color);
            DrawCylinderWires(Vector3{0, 0, 0}, object.radius, object.radius, object.height, 16, BLACK);
            break;

        case MapObjectType::PLANE:
            // Draw plane as a flat cube
            DrawCube(Vector3{0, 0, 0}, object.size.x, 0.1f, object.size.y, object.color);
            DrawCubeWires(Vector3{0, 0, 0}, object.size.x, 0.1f, object.size.y, BLACK);
            break;

        case MapObjectType::MODEL:
            // Find the corresponding loaded model for this object
            if (!object.modelName.empty() && !loadedModels.empty())
            {
                // Try to find a model that matches the modelName
                // For now, we'll use a simple approach - in a more sophisticated implementation,
                // you could store models with their names or use a map for lookup
                Model model = loadedModels[0]; // Use first model for now

                // Apply transformations to the model
                model.transform = transform;

                // Draw the model with the object's color as tint
                DrawModel(model, Vector3{0, 0, 0}, 1.0f, object.color);

                // Optional: Draw model wires for debugging
                // DrawModelWires(model, Vector3{0, 0, 0}, 1.0f, BLACK);
            }
            else
            {
                // No model name specified or no models loaded, draw placeholder
                DrawSphere(Vector3{0, 0, 0}, 0.5f, RED);
            }
            break;

        case MapObjectType::LIGHT:
            // Light objects don't render visually, they affect lighting
            DrawSphere(Vector3{0, 0, 0}, 0.2f, YELLOW); // Visual representation of light
            break;

        default:
            // Unknown object type - draw as cube
            DrawCube(Vector3{0, 0, 0}, 1.0f, 1.0f, 1.0f, object.color);
            DrawCubeWires(Vector3{0, 0, 0}, 1.0f, 1.0f, 1.0f, BLACK);
            break;
    }
}

// ============================================================================
// Design Pattern Implementations
// ============================================================================

// Concrete strategy implementations

class JsonMapLoaderStrategy : public IMapLoaderStrategy
{
public:
    GameMap LoadMap(const std::string& path) override
    {
        return LoadGameMap(path);
    }

    bool SaveMap(const GameMap& map, const std::string& path) override
    {
        return SaveGameMap(map, path);
    }

    std::string GetStrategyName() const override
    {
        return "JSON Map Loader";
    }
};

class FolderModelLoaderStrategy : public IModelLoaderStrategy
{
public:
    std::vector<ModelInfo> LoadModelsFromDirectory(const std::string& directory) override
    {
        std::vector<ModelInfo> models;
        std::set<std::string> supportedExtensions = {".glb", ".gltf", ".obj", ".fbx", ".dae"};

        try
        {
            if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
            {
                TraceLog(LOG_WARNING, "Directory does not exist or is not a directory: %s", directory.c_str());
                return models;
            }

            TraceLog(LOG_INFO, "Scanning directory for models: %s", directory.c_str());

            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
            {
                if (!entry.is_regular_file())
                    continue;

                std::string extension = entry.path().extension().string();
                std::string filename = entry.path().filename().string();

                // Skip hidden files and non-model files
                if (filename.starts_with(".") || supportedExtensions.find(extension) == supportedExtensions.end())
                    continue;

                std::string modelPath = entry.path().string();

                // Convert to relative path if within project directory
                std::string projectRoot = PROJECT_ROOT_DIR;
                if (modelPath.find(projectRoot) == 0)
                {
                    modelPath = modelPath.substr(projectRoot.length());
                }

                ModelInfo modelInfo;
                modelInfo.name = filename.substr(0, filename.find_last_of('.'));
                modelInfo.path = modelPath;
                modelInfo.extension = extension;

                // Determine properties based on file extension and name
                modelInfo.hasAnimations = (extension == ".glb" || extension == ".gltf");
                modelInfo.hasCollision = true;

                // Set default scale based on model name patterns
                if (modelInfo.name.find("player") != std::string::npos)
                    modelInfo.defaultScale = Vector3{0.01f, 0.01f, 0.01f};
                else if (modelInfo.name.find("tavern") != std::string::npos ||
                         modelInfo.name.find("arena") != std::string::npos)
                    modelInfo.defaultScale = Vector3{50.0f, 50.0f, 50.0f};
                else
                    modelInfo.defaultScale = Vector3{1.0f, 1.0f, 1.0f};

                models.push_back(modelInfo);

                TraceLog(LOG_INFO, "Found model: %s (%s)", modelInfo.name.c_str(), modelPath.c_str());
            }

            TraceLog(LOG_INFO, "Found %d models in directory: %s", models.size(), directory.c_str());
        }
        catch (const std::exception& e)
        {
            TraceLog(LOG_ERROR, "Error scanning models directory: %s", e.what());
        }

        return models;
    }

    bool SaveModelConfig(const std::vector<ModelInfo>& models, const std::string& path) override
    {
        json j = json::array();

        for (const auto& model : models)
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
            instance["scale"] = (model.defaultScale.x + model.defaultScale.y + model.defaultScale.z) / 3.0f;
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

    std::string GetStrategyName() const override
    {
        return "Folder Model Loader";
    }
};

// Observer pattern implementation
void MapLoaderSubject::AddObserver(IMapLoadObserver* observer)
{
    m_observers.push_back(observer);
}

void MapLoaderSubject::RemoveObserver(IMapLoadObserver* observer)
{
    m_observers.erase(std::remove(m_observers.begin(), m_observers.end(), observer), m_observers.end());
}

void MapLoaderSubject::NotifyMapLoaded(const std::string& mapName)
{
    for (auto* observer : m_observers)
    {
        observer->OnMapLoaded(mapName);
    }
}

void MapLoaderSubject::NotifyMapLoadFailed(const std::string& mapName, const std::string& error)
{
    for (auto* observer : m_observers)
    {
        observer->OnMapLoadFailed(mapName, error);
    }
}

// Enhanced MapLoader constructor
MapLoader::MapLoader()
{
    m_mapStrategy = MapLoaderFactory::CreateMapLoader("json");
    m_modelStrategy = MapLoaderFactory::CreateModelLoader("folder");
}

void MapLoader::SetMapLoaderStrategy(std::unique_ptr<IMapLoaderStrategy> strategy)
{
    m_mapStrategy = std::move(strategy);
}

void MapLoader::SetModelLoaderStrategy(std::unique_ptr<IModelLoaderStrategy> strategy)
{
    m_modelStrategy = std::move(strategy);
}

GameMap MapLoader::LoadMap(const std::string& path)
{
    try
    {
        GameMap map = m_mapStrategy->LoadMap(path);
        NotifyMapLoaded(path);
        return map;
    }
    catch (const std::exception& e)
    {
        NotifyMapLoadFailed(path, e.what());
        return GameMap{};
    }
}

bool MapLoader::SaveMap(const GameMap& map, const std::string& path)
{
    return m_mapStrategy->SaveMap(map, path);
}

std::vector<ModelInfo> MapLoader::LoadModelsFromDirectory(const std::string& directory)
{
    return m_modelStrategy->LoadModelsFromDirectory(directory);
}

bool MapLoader::SaveModelConfig(const std::vector<ModelInfo>& models, const std::string& path)
{
    return m_modelStrategy->SaveModelConfig(models, path);
}

// Factory implementation
std::unique_ptr<IMapLoaderStrategy> MapLoaderFactory::CreateMapLoader(const std::string& type)
{
    if (type == "json")
        return std::make_unique<JsonMapLoaderStrategy>();
    return std::make_unique<JsonMapLoaderStrategy>(); // Default
}

std::unique_ptr<IModelLoaderStrategy> MapLoaderFactory::CreateModelLoader(const std::string& type)
{
    if (type == "folder")
        return std::make_unique<FolderModelLoaderStrategy>();
    return std::make_unique<FolderModelLoaderStrategy>(); // Default
}

