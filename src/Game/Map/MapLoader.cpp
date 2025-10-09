#include "MapLoader.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <raymath.h>
#include <iostream>

using json = nlohmann::json;

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
        MapLoader mo;
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

        objects.push_back(mo);
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
        RenderMapObject(object, camera);
    }

    EndMode3D();
}

void RenderMapObject(const MapObjectData& object, [[maybe_unused]] Camera3D camera)
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

    // Temporarily apply transformation for rendering
    //rlPushMatrix();
    //rlMultMatrixf(MatrixToFloat(transform));

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
            // For model objects, we would need to load and render the actual model
            // This is a placeholder - in a real implementation, you'd want to load
            // the model during map loading and store it with the object data
            DrawSphere(Vector3{0, 0, 0}, 0.5f, RED); // Placeholder for model
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

    //rlPopMatrix();
}
