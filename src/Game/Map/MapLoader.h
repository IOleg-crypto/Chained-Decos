#ifndef MAPLOADER_H
#define MAPLOADER_H

#include <raylib.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Object types for the map
enum class MapObjectType
{
    CUBE = 0,
    SPHERE = 1,
    CYLINDER = 2,
    PLANE = 3,
    MODEL = 4,
    LIGHT = 5
};

// Enhanced map object structure for editor-created maps
struct MapObjectData
{
    std::string name;
    MapObjectType type;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    Color color;
    std::string modelName; // For MODEL type objects

    // Shape-specific properties
    float radius; // For spheres
    float height; // For cylinders
    Vector2 size; // For planes

    MapObjectData()
        : name(""), type(MapObjectType::CUBE), position{0, 0, 0}, rotation{0, 0, 0},
          scale{1, 1, 1}, color{WHITE}, modelName(""), radius(1.0f), height(1.0f), size{1, 1}
    {}
};

// Map metadata
struct MapMetadata
{
    std::string name;
    std::string displayName;
    std::string description;
    std::string author;
    std::string version;
    Vector3 startPosition;
    Vector3 endPosition;
    Color skyColor;
    Color groundColor;
    float difficulty;

    // Additional fields for JSON export compatibility
    std::string createdDate;
    std::string modifiedDate;
    Vector3 worldBounds;
    Color backgroundColor;
    std::string skyboxTexture;

    MapMetadata()
        : name(""), displayName(""), description(""), author(""), version("1.0"),
          startPosition{0, 0, 0}, endPosition{0, 0, 0}, skyColor{SKYBLUE},
          groundColor{DARKGREEN}, difficulty(1.0f), createdDate(""), modifiedDate(""),
          worldBounds{100.0f, 100.0f, 100.0f}, backgroundColor{50, 50, 50, 255}, skyboxTexture("")
    {}
};

// Complete map structure (moved up for strategy interfaces)
struct GameMap
{
    MapMetadata metadata;
    std::vector<MapObjectData> objects;
    std::vector<Model> loadedModels; // For cleanup

    GameMap() = default;
    ~GameMap() { Cleanup(); }

    void Cleanup();

};

// Model information structure for folder-based loading
struct ModelInfo
{
    std::string name;
    std::string path;
    std::string extension;
    bool hasAnimations;
    bool hasCollision;
    Vector3 defaultScale;

    ModelInfo() : hasAnimations(false), hasCollision(true), defaultScale{1.0f, 1.0f, 1.0f} {}
};

// Simple MapLoader class - direct implementation without patterns
class MapLoader
{
public:
    MapLoader() = default;

    GameMap LoadMap(const std::string& path);
    bool SaveMap(const GameMap& map, const std::string& path);
    std::vector<ModelInfo> LoadModelsFromDirectory(const std::string& directory);
    bool SaveModelConfig(const std::vector<ModelInfo>& models, const std::string& path);
};



// Map loading and saving functions
GameMap LoadGameMap(const std::string &path);
GameMap LoadGameMapFromModelsFormat(const nlohmann::json& j, const std::string& path);
GameMap LoadGameMapFromEditorFormat(const nlohmann::json& j, const std::string& path);
bool SaveGameMap(const GameMap& map, const std::string& path);

// Utility functions
MapObjectData CreateMapObjectFromType(MapObjectType type, const Vector3& position, const Vector3& scale, const Color& color);

// Map rendering functions
void RenderGameMap(const GameMap& map, Camera3D camera);
void RenderMapObject(const MapObjectData& object, const std::vector<Model>& loadedModels, Camera3D camera);

#endif // MAPLOADER_H
