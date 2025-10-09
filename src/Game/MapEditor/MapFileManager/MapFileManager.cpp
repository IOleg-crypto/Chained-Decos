//
// Created by I#Oleg
//

#include "MapFileManager.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool MapFileManager::SaveMap(const std::vector<SerializableObject> &objects,
                             const std::string &filename)
{
    json j;

    // Save objects array
    json objectsArray = json::array();
    for (const auto &obj : objects)
    {
        json object;
        object["name"] = obj.name;
        object["type"] = obj.type;

        // Position
        object["position"] = {
            {"x", obj.position.x},
            {"y", obj.position.y},
            {"z", obj.position.z}
        };

        // Scale
        object["scale"] = {
            {"x", obj.scale.x},
            {"y", obj.scale.y},
            {"z", obj.scale.z}
        };

        // Rotation
        object["rotation"] = {
            {"x", obj.rotation.x},
            {"y", obj.rotation.y},
            {"z", obj.rotation.z}
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

        objectsArray.push_back(object);
    }

    j["objects"] = objectsArray;

    // Write to file with pretty formatting
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }

    file << j.dump(4); // Pretty print with 4 spaces indentation
    file.close();
    std::cout << "Map saved successfully to: " << filename << std::endl;
    return true;
}

bool MapFileManager::LoadMap(std::vector<SerializableObject> &objects, const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }

    objects.clear();

    json j;
    try
    {
        file >> j;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to parse map JSON: " << e.what() << std::endl;
        return false;
    }

    // Load objects
    if (j.contains("objects"))
    {
        for (const auto& obj : j["objects"])
        {
            SerializableObject objectData;

            // Basic properties
            objectData.name = obj.value("name", "unnamed_object");
            objectData.type = obj.value("type", 0);

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

            objects.push_back(objectData);
        }
    }

    file.close();
    std::cout << "Map loaded successfully from: " << filename << std::endl;
    return true;
}