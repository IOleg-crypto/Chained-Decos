#ifndef MAPLOADER_H
#define MAPLOADER_H

#include "Engine/Map/MapData.h"
#include "Engine/Map/Skybox/skybox.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================================
// Data Structures
// ============================================================================


struct ModelInfo
{
    std::string name;
    std::string path;
    std::string extension;
    std::string category;    // Type of model (Player, Building, Environment, etc.)
    std::string description; // Human-readable description
    bool hasAnimations;
    bool hasCollision;
    Vector3 defaultScale;
};

// ============================================================================
// GameMap Class
// ============================================================================

// Forward declarations for friend classes
class MapManager;
class FileManager;

class GameMap
{
    // Friend functions for direct access to private members
    friend GameMap LoadGameMap(const std::string& path);
    friend GameMap LoadGameMapFromModelsFormat(const nlohmann::json& j, const std::string& path);
    friend GameMap LoadGameMapFromEditorFormat(const nlohmann::json& j, const std::string& path);
    friend bool SaveGameMap(const GameMap& map, const std::string& path);
    friend void LoadSkyboxForMap(GameMap& map);
    friend void RenderGameMap(const GameMap& map, Camera3D camera);
    friend void RenderMapObject(const MapObjectData& object,
                                const std::unordered_map<std::string, Model>& loadedModels,
                                Camera3D camera,
                                bool useEditorColors);
    //friend std::vector<GameMap> MapLoader::LoadAllMapsFromDirectory(const std::string& directory);
    
    // Friend classes for direct access to private members
    friend class MapManager;
    friend class FileManager;

public:
    GameMap() = default;
    ~GameMap();

    // Non-copyable, movable
    GameMap(const GameMap&) = delete;
    GameMap& operator=(const GameMap&) = delete;
    GameMap(GameMap&&) noexcept = default;
    GameMap& operator=(GameMap&&) noexcept = default;

    void Cleanup();

    // Skybox - оновлені методи
    void SetSkyBox(std::shared_ptr<Skybox>& skybox);
    Skybox* GetSkyBox() const;

    // Models
    const std::unordered_map<std::string, Model>& GetMapModels() const;
    void AddMapModels(const std::unordered_map<std::string, Model>& modelsMap);

    // Objects
    const std::vector<MapObjectData>& GetMapObjects() const;
    void AddMapObjects(const std::vector<MapObjectData>& mapObjects);

    // Metadata
    const MapMetadata& GetMapMetaData() const;
    void SetMapMetaData(const MapMetadata& mapData);

private:
    MapMetadata m_metadata;
    std::vector<MapObjectData> m_objects;
    std::unordered_map<std::string, Model> m_loadedModels;
    std::shared_ptr<Skybox> m_skybox;  // Змінено на новий клас Skybox
};

// ============================================================================
// MapLoader Class
// ============================================================================

class MapLoader
{
public:
    MapLoader() = default;
    ~MapLoader() = default;

    // Non-copyable, movable
    MapLoader(const MapLoader&) = delete;
    MapLoader& operator=(const MapLoader&) = delete;
    MapLoader(MapLoader&&) noexcept = default;
    MapLoader& operator=(MapLoader&&) noexcept = default;

    GameMap LoadMap(const std::string& path);
    bool SaveMap(const GameMap& map, const std::string& path);
    std::vector<ModelInfo> LoadModelsFromDirectory(const std::string& directory);
    bool SaveModelConfig(const std::vector<ModelInfo>& models, const std::string& path);

    // Load all maps from a directory
    std::vector<GameMap> LoadAllMapsFromDirectory(const std::string& directory);

    // Get map names from directory
    std::vector<std::string> GetMapNamesFromDirectory(const std::string& directory);
};

// ============================================================================
// Free Functions - Map Loading/Saving
// ============================================================================

GameMap LoadGameMap(const std::string& path);
GameMap LoadGameMapFromModelsFormat(const nlohmann::json& j, const std::string& path);
GameMap LoadGameMapFromEditorFormat(const nlohmann::json& j, const std::string& path);
bool SaveGameMap(const GameMap& map, const std::string& path);
void LoadSkyboxForMap(GameMap& map);

// ============================================================================
// Utility Functions
// ============================================================================

MapObjectData CreateMapObjectFromType(MapObjectType type, const Vector3& position,
                                      const Vector3& scale, const Color& color);

// ============================================================================
// Rendering Functions
// ============================================================================

void RenderGameMap(const GameMap& map, Camera3D camera);
void RenderMapObject(const MapObjectData& object,
                     const std::unordered_map<std::string, Model>& loadedModels,
                     Camera3D camera,
                     bool useEditorColors = false);

#endif // MAPLOADER_H
