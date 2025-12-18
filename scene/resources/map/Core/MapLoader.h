#ifndef MAPLOADER_H
#define MAPLOADER_H

#pragma once

#include "MapData.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "scene/resources/map/Skybox/Skybox.h"

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

class GameMap
{

public:
    GameMap() = default;
    ~GameMap();

    // Non-copyable, movable
    GameMap(const GameMap &) = delete;
    GameMap &operator=(const GameMap &) = delete;
    GameMap(GameMap &&) noexcept = default;
    GameMap &operator=(GameMap &&) noexcept = default;

    void Cleanup();

    // Skybox - updated methods
    void SetSkyBox(std::shared_ptr<Skybox> &skybox);
    Skybox *GetSkyBox() const;

    // Models
    const std::unordered_map<std::string, Model> &GetMapModels() const;
    void AddMapModels(const std::unordered_map<std::string, Model> &modelsMap);
    std::unordered_map<std::string, Model> &GetMapModelsMutable();

    // Objects
    const std::vector<MapObjectData> &GetMapObjects() const;
    void AddMapObjects(const std::vector<MapObjectData> &mapObjects);
    std::vector<MapObjectData> &GetMapObjectsMutable();

    // Metadata
    const MapMetadata &GetMapMetaData() const;
    void SetMapMetaData(const MapMetadata &mapData);
    MapMetadata &GetMapMetaDataMutable();

private:
    MapMetadata m_metadata;
    std::vector<MapObjectData> m_objects;
    std::unordered_map<std::string, Model> m_loadedModels;
    std::shared_ptr<Skybox> m_skybox;
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
    MapLoader(const MapLoader &) = delete;
    MapLoader &operator=(const MapLoader &) = delete;
    MapLoader(MapLoader &&) noexcept = default;
    MapLoader &operator=(MapLoader &&) noexcept = default;

    GameMap LoadMap(const std::string &path);
    bool SaveMap(const GameMap &map, const std::string &path);
    std::vector<ModelInfo> LoadModelsFromDirectory(const std::string &directory);
    bool SaveModelConfig(const std::vector<ModelInfo> &models, const std::string &path);

    // Load all maps from a directory
    std::vector<GameMap> LoadAllMapsFromDirectory(const std::string &directory);

    // Get map names from directory
    std::vector<std::string> GetMapNamesFromDirectory(const std::string &directory);

    // Skybox operations
    void LoadSkyboxForMap(GameMap &map);

private:
    bool SaveMapToFile(const GameMap &map, const std::string &path);
};

// ============================================================================
// Utility Functions
// ============================================================================

MapObjectData CreateMapObjectFromType(MapObjectType type, const Vector3 &position,
                                      const Vector3 &scale, const Color &color);

#endif // MAPLOADER_H
