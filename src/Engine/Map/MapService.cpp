//
// Created by AI Assistant
//

#include "MapService.h"
#include "MapLoader.h"
#include "MapObjectConverter.h"
#include "../MapFileManager/JsonMapFileManager.h"
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;

bool MapService::LoadMap(const std::string& filename,
                         std::vector<MapObjectData>& objects,
                         MapMetadata& metadata)
{
    std::vector<JsonSerializableObject> jsonObjects;
    if (!JsonMapFileManager::LoadMap(jsonObjects, filename, metadata))
    {
        std::cout << "MapService::LoadMap() - Failed to load map: " << filename << std::endl;
        return false;
    }
    
    objects = ConvertJsonObjectsToMapObjects(jsonObjects);
    
    std::cout << "MapService::LoadMap() - Successfully loaded map: " << filename 
              << " with " << objects.size() << " objects" << std::endl;
    return true;
}

bool MapService::SaveMap(const std::string& filename,
                         const std::vector<MapObjectData>& objects,
                         const MapMetadata& metadata)
{
    std::vector<JsonSerializableObject> jsonObjects = ConvertMapObjectsToJsonObjects(objects);
    
    if (!JsonMapFileManager::SaveMap(jsonObjects, filename, metadata))
    {
        std::cout << "MapService::SaveMap() - Failed to save map: " << filename << std::endl;
        return false;
    }
    
    std::cout << "MapService::SaveMap() - Successfully saved map: " << filename 
              << " with " << objects.size() << " objects" << std::endl;
    return true;
}

GameMap MapService::LoadMapAsGameMap(const std::string& filename)
{
    return m_mapLoader.LoadMap(filename);
}

bool MapService::SaveGameMap(const std::string& filename, const GameMap& gameMap)
{
    return m_mapLoader.SaveMap(gameMap, filename);
}

std::vector<MapObjectData> MapService::ConvertJsonObjectsToMapObjects(
    const std::vector<JsonSerializableObject>& jsonObjects)
{
    std::vector<MapObjectData> mapObjects;
    mapObjects.reserve(jsonObjects.size());
    
    for (const auto& jsonObj : jsonObjects)
    {
        mapObjects.push_back(MapObjectConverter::JsonSerializableObjectToMapObjectData(jsonObj));
    }
    
    return mapObjects;
}

std::vector<JsonSerializableObject> MapService::ConvertMapObjectsToJsonObjects(
    const std::vector<MapObjectData>& mapObjects)
{
    std::vector<JsonSerializableObject> jsonObjects;
    jsonObjects.reserve(mapObjects.size());
    
    for (const auto& mapObj : mapObjects)
    {
        jsonObjects.push_back(MapObjectConverter::MapObjectDataToJsonSerializableObject(mapObj));
    }
    
    return jsonObjects;
}

MapMetadata MapService::GetDefaultMetadata() const
{
    return JsonMapFileManager::CreateDefaultMetadata();
}

MapMetadata MapService::CreateMetadataFromName(const std::string& mapName) const
{
    MapMetadata metadata = GetDefaultMetadata();
    fs::path path(mapName);
    std::string name = path.stem().string();
    
    metadata.name = name;
    metadata.displayName = name;
    
    return metadata;
}

bool MapService::ValidateMapFile(const std::string& filename) const
{
    return JsonMapFileManager::ValidateMapFile(filename);
}

std::string MapService::GetMapVersion(const std::string& filename) const
{
    return JsonMapFileManager::GetMapVersion(filename);
}

std::vector<GameMap> MapService::LoadAllMapsFromDirectory(const std::string& directory)
{
    return m_mapLoader.LoadAllMapsFromDirectory(directory);
}

std::vector<std::string> MapService::GetMapNamesFromDirectory(const std::string& directory)
{
    return m_mapLoader.GetMapNamesFromDirectory(directory);
}

void MapService::RenderMap(const GameMap& gameMap, Camera3D camera)
{
    m_mapLoader.RenderMap(gameMap, camera);
}

void MapService::RenderMapObject(const MapObjectData& object,
                                 const std::unordered_map<std::string, Model>& loadedModels,
                                 Camera3D camera,
                                 bool useEditorColors)
{
    m_mapLoader.RenderMapObject(object, loadedModels, camera, useEditorColors);
}

Vector3 MapService::ExtractSpawnPosition(const std::vector<MapObjectData>& objects) const
{
    for (const auto& obj : objects)
    {
        if (obj.type == MapObjectType::SPAWN_ZONE)
        {
            return obj.position;
        }
    }
    
    return {0.0f, 2.0f, 0.0f};
}

MapObjectData MapService::CreateSpawnZoneObject(const Vector3& position) const
{
    MapObjectData spawnZone;
    spawnZone.name = "Spawn Zone";
    spawnZone.type = MapObjectType::SPAWN_ZONE;
    spawnZone.position = position;
    spawnZone.scale = {2.0f, 2.0f, 2.0f};
    spawnZone.color = {255, 0, 255, 128};
    spawnZone.isPlatform = false;
    spawnZone.isObstacle = false;
    
    return spawnZone;
}

