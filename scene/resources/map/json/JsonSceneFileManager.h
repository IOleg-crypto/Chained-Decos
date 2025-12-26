#pragma once

#include "json/single_include/nlohmann/json.hpp"
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
    int type;

    // Additional fields
    bool visible = true;
    std::string layer = "default";
    std::string tags = "";
    std::string color = "white";

    // Sphere/shape properties
    float radiusSphere = 0.0f;
    float radiusH = 0.0f;
    float radiusV = 0.0f;
    Vector2 size = {0.0f, 0.0f};

    // Convert to/from JSON
    json ToJson() const;
    static JsonSerializableObject FromJson(const json &j);
};

// JSON Scene File Manager
class JsonSceneFileManager
{
public:
    JsonSceneFileManager() = default;
    ~JsonSceneFileManager() = default;

    // Load map from JSON file
    bool LoadScene(const std::string &filepath, std::vector<JsonSerializableObject> &objects);

    // Save map to JSON file
    bool SaveScene(const std::string &filepath, const std::vector<JsonSerializableObject> &objects);
};
