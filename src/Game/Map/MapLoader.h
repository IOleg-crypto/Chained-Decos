#ifndef MAPLOADER_H
#define MAPLOADER_H

#include <raylib.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

// Forward declarations for Strategy Pattern
class IMapLoaderStrategy;
class IModelLoaderStrategy;

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

// Factory for creating loader strategies
class MapLoaderFactory
{
public:
    static std::unique_ptr<IMapLoaderStrategy> CreateMapLoader(const std::string& type);
    static std::unique_ptr<IModelLoaderStrategy> CreateModelLoader(const std::string& type);
};

// Strategy interface for map loading
class IMapLoaderStrategy
{
public:
    virtual ~IMapLoaderStrategy() = default;
    virtual GameMap LoadMap(const std::string& path) = 0;
    virtual bool SaveMap(const GameMap& map, const std::string& path) = 0;
    virtual std::string GetStrategyName() const = 0;
};

// Strategy interface for model loading
class IModelLoaderStrategy
{
public:
    virtual ~IModelLoaderStrategy() = default;
    virtual std::vector<ModelInfo> LoadModelsFromDirectory(const std::string& directory) = 0;
    virtual bool SaveModelConfig(const std::vector<ModelInfo>& models, const std::string& path) = 0;
    virtual std::string GetStrategyName() const = 0;
};

// Observer interface for map loading notifications
class IMapLoadObserver
{
public:
    virtual ~IMapLoadObserver() = default;
    virtual void OnMapLoaded(const std::string& mapName) = 0;
    virtual void OnMapLoadFailed(const std::string& mapName, const std::string& error) = 0;
};

// Subject class for managing observers
class MapLoaderSubject
{
private:
    std::vector<IMapLoadObserver*> m_observers;
    
public:
    void AddObserver(IMapLoadObserver* observer);
    void RemoveObserver(IMapLoadObserver* observer);
    void NotifyMapLoaded(const std::string& mapName);
    void NotifyMapLoadFailed(const std::string& mapName, const std::string& error);
};

// Enhanced MapLoader class using Strategy pattern
class MapLoader : public MapLoaderSubject
{
private:
    std::unique_ptr<IMapLoaderStrategy> m_mapStrategy;
    std::unique_ptr<IModelLoaderStrategy> m_modelStrategy;
    
public:
    MapLoader();
    
    void SetMapLoaderStrategy(std::unique_ptr<IMapLoaderStrategy> strategy);
    void SetModelLoaderStrategy(std::unique_ptr<IModelLoaderStrategy> strategy);
    
    GameMap LoadMap(const std::string& path);
    bool SaveMap(const GameMap& map, const std::string& path);
    std::vector<ModelInfo> LoadModelsFromDirectory(const std::string& directory);
    bool SaveModelConfig(const std::vector<ModelInfo>& models, const std::string& path);
};



// Legacy support - old MapLoader structure
struct LegacyMapLoader
{
    std::string modelName;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    Model loadedModel;

    LegacyMapLoader()
        : modelName(""), position{0, 0, 0}, rotation{0, 0, 0}, scale{1, 1, 1}, loadedModel{0}
    {}
};

// Legacy function for backward compatibility
std::vector<MapLoader> LoadMap(const std::string &path);

// New comprehensive map loading functions
GameMap LoadGameMap(const std::string &path);
GameMap LoadModelsMap(const std::string& path); // For array-based model format
bool SaveGameMap(const GameMap& map, const std::string& path);
bool SaveModelsMap(const GameMap& map, const std::string& path); // Export in models.json format
bool ExportMapForEditor(const GameMap& map, const std::string& path);

// Utility functions
MapObjectData CreateMapObjectFromType(MapObjectType type, const Vector3& position, const Vector3& scale, const Color& color);

// Map rendering functions
void RenderGameMap(const GameMap& map, Camera3D camera);
void RenderMapObject(const MapObjectData& object, const std::vector<Model>& loadedModels, Camera3D camera);

#endif // MAPLOADER_H
