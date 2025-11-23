#pragma once

#include <nlohmann/json.hpp>
#include <raylib.h>
#include <string>
#include <vector>

using json = nlohmann::json;

// Serializable object for JSON
struct JsonSerializableObject
{
    std::string id;
    std::string name;
    std::string modelName;
    Vector3 position;
    Vector3 rotation;
    float scale;
    // Convert to/from JSON
    json ToJson() const;
    static JsonSerializableObject FromJson(const json &j);
};

// JSON Map File Manager
class JsonMapFileManager
{
public:
    JsonMapFileManager() = default;
    ~JsonMapFileManager() = default;

    // Load map from JSON file
    bool LoadMap(const std::string &filepath, std::vector<JsonSerializableObject> &objects);

    // Save map to JSON file
    bool SaveMap(const std::string &filepath, const std::vector<JsonSerializableObject> &objects);
};
