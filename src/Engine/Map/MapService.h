//
// Created by AI Assistant
// MapService - Unified service for map operations used by both Editor and Game
//

#ifndef MAPSERVICE_H
#define MAPSERVICE_H

#include "MapData.h"
#include "MapLoader.h"
#include "../MapFileManager/JsonMapFileManager.h"
#include <string>
#include <vector>
#include <unordered_map>

// Unified service for map operations used by both Editor and Game
class MapService
{
public:
    MapService() = default;
    ~MapService() = default;
    
    // Non-copyable, movable
    MapService(const MapService&) = delete;
    MapService& operator=(const MapService&) = delete;
    MapService(MapService&&) noexcept = default;
    MapService& operator=(MapService&&) noexcept = default;
    
    bool LoadMap(const std::string& filename, 
                 std::vector<MapObjectData>& objects, 
                 MapMetadata& metadata);
    
    bool SaveMap(const std::string& filename,
                 const std::vector<MapObjectData>& objects,
                 const MapMetadata& metadata);
    
    GameMap LoadMapAsGameMap(const std::string& filename);
    bool SaveGameMap(const std::string& filename, const GameMap& gameMap);
    
    std::vector<MapObjectData> ConvertJsonObjectsToMapObjects(
        const std::vector<JsonSerializableObject>& jsonObjects);
    
    std::vector<JsonSerializableObject> ConvertMapObjectsToJsonObjects(
        const std::vector<MapObjectData>& mapObjects);
    
    MapMetadata GetDefaultMetadata() const;
    MapMetadata CreateMetadataFromName(const std::string& mapName) const;
    
    bool ValidateMapFile(const std::string& filename) const;
    std::string GetMapVersion(const std::string& filename) const;
    
    std::vector<GameMap> LoadAllMapsFromDirectory(const std::string& directory);
    std::vector<std::string> GetMapNamesFromDirectory(const std::string& directory);
    
    void RenderMap(const GameMap& gameMap, Camera3D camera);
    void RenderMapObject(const MapObjectData& object,
                        const std::unordered_map<std::string, Model>& loadedModels,
                        Camera3D camera,
                        bool useEditorColors = false);

private:
    MapLoader m_mapLoader;
    Vector3 ExtractSpawnPosition(const std::vector<MapObjectData>& objects) const;
    MapObjectData CreateSpawnZoneObject(const Vector3& position) const;
};

#endif // MAPSERVICE_H

