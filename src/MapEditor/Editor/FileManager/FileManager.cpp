//
// Created by Kilo Code
//

#include "FileManager.h"
#include "MapObjectConverterEditor.h"
#include "../Object/MapObject.h"
#include "Engine/Map/MapService.h"
#include "Engine/Map/MapLoader.h"
#include <filesystem>
#include <iostream>
#include <raylib.h>

namespace fs = std::filesystem;

FileManager::FileManager()
    : m_currentlyLoadedMapFilePath(""), m_currentMetadata()
{
    MapMetadata defaultMeta;
    defaultMeta.name = "Untitled Map";
    defaultMeta.displayName = "Untitled Map";
    defaultMeta.skyColor = SKYBLUE;
    m_currentMetadata = defaultMeta;
}

FileManager::~FileManager()
{
}

bool FileManager::SaveMap(const std::string &filename, const std::vector<MapObject> &objects)
{
    Vector3 spawnPosition = {0.0f, 2.0f, 0.0f};
    
    GameMap gameMap;
    std::vector<MapObjectData>& mapObjects = gameMap.GetMapObjectsMutable();
    for (const auto &obj : objects)
    {
        if (obj.GetObjectType() == 6)
        {
            spawnPosition = obj.GetPosition();
            continue;
        }
        mapObjects.push_back(MapObjectConverterEditor::MapObjectToMapObjectData(obj));
    }

    MapMetadata metadata = m_currentMetadata;
    if (metadata.name.empty())
    {
        metadata.name = fs::path(filename).stem().string();
        metadata.displayName = metadata.name;
    }
    metadata.startPosition = spawnPosition;

    if (metadata.skyboxTexture.empty() && metadata.skyColor.a == 0)
    {
        metadata.skyColor = SKYBLUE;
    }

    gameMap.SetMapMetaData(metadata);

    if (m_mapService.SaveMap(filename, gameMap))
    {
        std::cout << "Map saved successfully to " << filename << std::endl;
        m_currentlyLoadedMapFilePath = filename;
        m_currentMetadata = metadata;
        return true;
    }

    std::cout << "Failed to save map to " << filename << std::endl;
    return false;
}

bool FileManager::LoadMap(const std::string &filename, std::vector<MapObject> &objects)
{
    GameMap gameMap;
    if (!m_mapService.LoadMap(filename, gameMap))
    {
        std::cout << "Failed to load map: " << filename << std::endl;
        return false;
    }

    const MapMetadata& metadata = gameMap.GetMapMetaData();
    m_currentMetadata = metadata;

    objects.clear();
    for (const auto &data : gameMap.GetMapObjects())
    {
        objects.push_back(MapObjectConverterEditor::MapObjectDataToMapObject(data));
    }

    if (metadata.startPosition.y >= 2.0f ||
        (metadata.startPosition.x != 0.0f || metadata.startPosition.y != 0.0f ||
         metadata.startPosition.z != 0.0f))
    {
        MapObject spawnZone;
        spawnZone.SetObjectType(6);
        spawnZone.SetPosition(metadata.startPosition);
        spawnZone.SetObjectName("Spawn Zone");
        spawnZone.SetColor({255, 0, 255, 128});
        spawnZone.SetScale({2.0f, 2.0f, 2.0f});
        spawnZone.SetSelected(false);
        objects.push_back(spawnZone);
    }

    std::cout << "Map loaded successfully from " << filename << std::endl;
    m_currentlyLoadedMapFilePath = filename;
    return true;
}

std::string FileManager::GetCurrentlyLoadedMapFilePath() const
{
    return m_currentlyLoadedMapFilePath;
}

void FileManager::SetCurrentlyLoadedMapFilePath(const std::string &path)
{
    m_currentlyLoadedMapFilePath = path;
}

void FileManager::SetSkyboxTexture(const std::string &path)
{
    m_currentMetadata.skyboxTexture = path;
}

void FileManager::SetCurrentMetadata(const MapMetadata& metadata)
{
    m_currentMetadata = metadata;
}
