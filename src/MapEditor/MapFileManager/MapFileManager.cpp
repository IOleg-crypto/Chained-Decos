//
// Created by I#Oleg
//

#include "../MapFileManager/MapFileManager.h"
#include <raylib.h>

nlohmann::json SerializableObject::toJson() const {
    // Convert SerializableObject to JSON format
    nlohmann::json j;
    j["position"] = MapFileManager::Vector3ToJson(position);
    j["scale"] = MapFileManager::Vector3ToJson(scale);
    j["rotation"] = MapFileManager::Vector3ToJson(rotation);
    j["color"] = MapFileManager::ColorToJson(color);
    j["name"] = name;
    j["type"] = type;
    return j;
}

SerializableObject SerializableObject::fromJson(const nlohmann::json& j) {
    // Create SerializableObject from JSON data
    SerializableObject obj;
    obj.position = MapFileManager::JsonToVector3(j["position"]);
    obj.scale = MapFileManager::JsonToVector3(j["scale"]);
    obj.rotation = MapFileManager::JsonToVector3(j["rotation"]);
    obj.color = MapFileManager::JsonToColor(j["color"]);
    obj.name = j["name"];
    obj.type = j["type"];
    return obj;
}

bool MapFileManager::SaveMap(const std::vector<SerializableObject>& objects, const std::string& filename) {
    try {
        // Create JSON structure for the map
        nlohmann::json mapData;
        mapData["version"] = "1.0";
        mapData["objectCount"] = objects.size();
        
        // Convert all objects to JSON array
        nlohmann::json objectsArray = nlohmann::json::array();
        for (const auto& obj : objects) {
            objectsArray.push_back(obj.toJson());
        }
        mapData["objects"] = objectsArray;
        
        // Write JSON to file
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return false;
        }
        
        file << mapData.dump(4);  // Pretty print with 4 spaces indentation
        file.close();
        
        std::cout << "Map saved successfully to: " << filename << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving map: " << e.what() << std::endl;
        return false;
    }
}

bool MapFileManager::LoadMap(std::vector<SerializableObject>& objects, const std::string& filename) {
    try {
        // Read JSON from file
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filename << std::endl;
            return false;
        }
        
        nlohmann::json mapData;
        file >> mapData;
        file.close();
        
        // Clear existing objects and load new ones
        objects.clear();
        const auto& objectsArray = mapData["objects"];
        for (const auto& objJson : objectsArray) {
            objects.push_back(SerializableObject::fromJson(objJson));
        }
        
        std::cout << "Map loaded successfully from: " << filename << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading map: " << e.what() << std::endl;
        return false;
    }
}

nlohmann::json MapFileManager::Vector3ToJson(const Vector3& vec) {
    // Convert Vector3 to JSON object
    nlohmann::json j;
    j["x"] = vec.x;
    j["y"] = vec.y;
    j["z"] = vec.z;
    return j;
}

Vector3 MapFileManager::JsonToVector3(const nlohmann::json& j) {
    // Convert JSON object to Vector3
    return Vector3{
        j["x"].get<float>(),
        j["y"].get<float>(),
        j["z"].get<float>()
    };
}

nlohmann::json MapFileManager::ColorToJson(const Color& color) {
    // Convert Color to JSON object
    nlohmann::json j;
    j["r"] = color.r;
    j["g"] = color.g;
    j["b"] = color.b;
    j["a"] = color.a;
    return j;
}

Color MapFileManager::JsonToColor(const nlohmann::json& j) {
    // Convert JSON object to Color
    return Color{
        j["r"].get<unsigned char>(),
        j["g"].get<unsigned char>(),
        j["b"].get<unsigned char>(),
        j["a"].get<unsigned char>()
    };
} 