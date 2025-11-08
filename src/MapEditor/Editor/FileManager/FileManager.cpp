//
// Created by Kilo Code
//

#include "FileManager.h"
#include "../Object/MapObject.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Map/MapLoader.h"
#include "Engine/MapFileManager/JsonMapFileManager.h"
#include "Engine/MapFileManager/MapFileManager.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <raylib.h>
#include <rlImGui.h>
#include <string>


#include <raymath.h>

namespace fs = std::filesystem;

FileManager::FileManager()
    : m_displayParkourMapDialog(false), m_currentlySelectedParkourMapIndex(0), m_currentMetadata()
{
    // NFD is initialized in Editor::InitializeSubsystems()
}

FileManager::~FileManager()
{
    // NFD cleanup is handled in Editor::~Editor()
}

// Helper functions to convert between MapObject (Editor) and MapObjectData (Engine)
static MapObjectData ConvertMapObjectToMapObjectData(const MapObject &obj)
{
    MapObjectData data;
    data.name      = obj.GetObjectName();
    data.position  = obj.GetPosition();
    data.rotation  = obj.GetRotation();
    data.scale     = obj.GetScale();
    data.color     = obj.GetColor();
    data.modelName = obj.GetModelAssetName();

    // Convert object type (Editor uses int, Engine uses enum)
    switch (obj.GetObjectType())
    {
    case 0:
        data.type = MapObjectType::CUBE;
        break;
    case 1:
        data.type = MapObjectType::SPHERE;
        break;
    case 2:
        data.type = MapObjectType::CYLINDER;
        break;
    case 3:
        data.type = MapObjectType::PLANE;
        break;
    case 4:
        data.type = MapObjectType::LIGHT;
        break;
    case 5:
        data.type = MapObjectType::MODEL;
        break;
    case 6:
        data.type = MapObjectType::SPAWN_ZONE;
        break;
    default:
        data.type = MapObjectType::CUBE;
        break;
    }

    // Shape-specific properties
    data.radius = obj.GetSphereRadius();
    data.height = obj.GetScale().y; // Use scale.y for cylinder height
    data.size   = obj.GetPlaneSize();

    // Default collision properties
    data.isPlatform = true;
    data.isObstacle = false;

    return data;
}

static MapObject ConvertMapObjectDataToMapObject(const MapObjectData &data)
{
    MapObject obj;
    obj.SetObjectName(data.name);
    obj.SetPosition(data.position);
    obj.SetRotation(data.rotation);
    obj.SetScale(data.scale);
    obj.SetColor(data.color);
    obj.SetModelAssetName(data.modelName);
    obj.SetSelected(false);

    // Convert object type
    switch (data.type)
    {
    case MapObjectType::CUBE:
        obj.SetObjectType(0);
        break;
    case MapObjectType::SPHERE:
        obj.SetObjectType(1);
        break;
    case MapObjectType::CYLINDER:
        obj.SetObjectType(2);
        break;
    case MapObjectType::PLANE:
        obj.SetObjectType(3);
        break;
    case MapObjectType::LIGHT:
        obj.SetObjectType(4);
        break;
    case MapObjectType::MODEL:
        obj.SetObjectType(5);
        break;
    case MapObjectType::SPAWN_ZONE:
        obj.SetObjectType(6);
        break;
    }

    // Shape-specific properties
    obj.SetSphereRadius(data.radius);
    obj.SetPlaneSize(data.size);

    return obj;
}

bool FileManager::SaveMap(const std::string &filename, const std::vector<MapObject> &objects)
{
    // Convert MapObjects to GameMap using shared MapLoader
    GameMap gameMap;

    gameMap.SetMapMetaData(m_currentMetadata);

    // Find spawn zone and set metadata.startPosition, but don't save it as a regular object
    Vector3 spawnPosition = {0.0f, 2.0f, 0.0f}; // Default spawn position
    bool hasSpawnZone     = false;

    // Convert MapObjects to MapObjectData (skip SPAWN_ZONE objects)
    for (const auto &obj : objects)
    {
        if (obj.GetObjectType() == 6) // SPAWN_ZONE
        {
            // Use spawn zone position for metadata
            spawnPosition = obj.GetPosition();
            hasSpawnZone  = true;
            // Don't add spawn zone as a regular object
            continue;
        }
        gameMap.AddMapObjects({ConvertMapObjectToMapObjectData(obj)});
    }

    gameMap.SetMapMetaData(MapMetadata{fs::path(filename).stem().string(),
                                       fs::path(filename).stem().string(),
                                       "",
                                       "",
                                       "1.0",
                                       {spawnPosition.x, spawnPosition.y, spawnPosition.z},
                                       {0.0f, 0.0f, 0.0f},
                                       SKYBLUE,
                                       DARKGREEN,
                                       1.0f,
                                       "2024-01-01T00:00:00Z",
                                       "2024-01-01T00:00:00Z",
                                       {100.0f, 100.0f, 100.0f},
                                       {50, 50, 50, 255},
                                       m_currentMetadata.skyboxTexture});

    // Save using shared MapLoader
    MapLoader loader;
    if (loader.SaveMap(gameMap, filename))
    {
        std::cout << "Map saved successfully using shared MapLoader!" << std::endl;
        m_currentlyLoadedMapFilePath = filename;
        return true;
    }
    else
    {
        std::cout << "Failed to save map!" << std::endl;
        return false;
    }
}

bool FileManager::LoadMap(const std::string &filename, std::vector<MapObject> &objects)
{
    // Load map using shared MapLoader
    MapLoader loader;
    GameMap gameMap = loader.LoadMap(filename);

    if (gameMap.GetMapObjects().empty() && gameMap.GetMapMetaData().name.empty())
    {
        // Try fallback to old format if MapLoader fails
        std::vector<SerializableObject> serializableObjects;
        if (MapFileManager::LoadMap(serializableObjects, filename))
        {
            objects.clear();
            TraceLog(LOG_INFO,
                     "FileManager::LoadMap() - Using fallback MapFileManager for compatibility");

            for (const auto &[position, scale, rotation, color, name, type, modelName] :
                 serializableObjects)
            {
                MapObject obj;
                obj.SetPosition(position);
                obj.SetScale(scale);
                obj.SetRotation(rotation);
                obj.SetColor(color);
                obj.SetObjectName(name);
                obj.SetObjectType(type);
                obj.SetModelAssetName(modelName);
                obj.SetSelected(false);
                objects.push_back(obj);
            }

            m_currentlyLoadedMapFilePath  = filename;
            m_currentMetadata             = MapMetadata();
            m_currentMetadata.name        = fs::path(filename).stem().string();
            m_currentMetadata.displayName = m_currentMetadata.name;
            return true;
        }

        std::cout << "Failed to load map!" << std::endl;
        return false;
    }

    // Clear current scene
    objects.clear();
    TraceLog(LOG_INFO, "FileManager::LoadMap() - Loaded map using shared MapLoader");
    m_currentMetadata = gameMap.GetMapMetaData();

    // Convert MapObjectData to MapObjects
    for (const auto &data : gameMap.GetMapObjects())
    {
        objects.push_back(ConvertMapObjectDataToMapObject(data));
    }

    // Restore spawn zone from metadata.startPosition if it exists
    // Check if startPosition is not at default position (0,0,0) or if it's explicitly set
    // We restore spawn zone if Y coordinate is >= 2.0f (typical spawn height) or if any coordinate
    // is non-zero
    if (gameMap.GetMapMetaData().startPosition.y >= 2.0f ||
        (gameMap.GetMapMetaData().startPosition.x != 0.0f ||
         gameMap.GetMapMetaData().startPosition.y != 0.0f ||
         gameMap.GetMapMetaData().startPosition.z != 0.0f))
    {
        MapObject spawnZone;
        spawnZone.SetObjectType(6); // SPAWN_ZONE
        spawnZone.SetPosition(gameMap.GetMapMetaData().startPosition);
        spawnZone.SetObjectName("Spawn Zone");
        spawnZone.SetColor({255, 0, 255, 128}); // Magenta with transparency
        spawnZone.SetScale({2.0f, 2.0f, 2.0f}); // Default spawn zone size
        spawnZone.SetSelected(false);
        objects.push_back(spawnZone);
        TraceLog(LOG_INFO,
                 "FileManager::LoadMap() - Restored spawn zone from metadata at (%.2f, %.2f, %.2f)",
                 gameMap.GetMapMetaData().startPosition.x, gameMap.GetMapMetaData().startPosition.y,
                 gameMap.GetMapMetaData().startPosition.z);
    }

    std::cout << "Map loaded successfully using shared MapLoader!" << std::endl;
    m_currentlyLoadedMapFilePath = filename;
    return true;
}

bool FileManager::ExportForGame(const std::string &filename, const std::vector<MapObject> &objects)
{
    // Convert MapObjects to JsonSerializableObjects for models.json export
    std::vector<JsonSerializableObject> jsonObjects;

    // Find spawn zone and set metadata.startPosition
    Vector3 spawnPosition = {0.0f, 2.0f, 0.0f}; // Default spawn position
    bool hasSpawnZone     = false;

    for (const auto &obj : objects)
    {
        // Handle spawn zone separately
        if (obj.GetObjectType() == 6) // SPAWN_ZONE
        {
            spawnPosition = obj.GetPosition();
            hasSpawnZone  = true;
            // Don't export spawn zone as a regular object
            continue;
        }

        JsonSerializableObject jsonObj;

        jsonObj.position  = obj.GetPosition();
        jsonObj.scale     = obj.GetScale();
        jsonObj.rotation  = obj.GetRotation();
        jsonObj.color     = obj.GetColor();
        jsonObj.name      = obj.GetObjectName();
        jsonObj.type      = obj.GetObjectType();
        jsonObj.modelName = obj.GetModelAssetName();
        jsonObj.visible   = true;
        jsonObj.layer     = "default";
        jsonObj.tags      = "exported";
        jsonObj.id =
            "obj_" + std::to_string(rand() % 9000 + 1000) + "_" + std::to_string(time(nullptr));

        // Set shape-specific properties for non-model objects
        switch (obj.GetObjectType())
        {
        case 1: // Sphere
            jsonObj.radiusSphere = obj.GetSphereRadius();
            break;
        case 2: // Cylinder
            jsonObj.radiusH = obj.GetScale().x;
            jsonObj.radiusV = obj.GetScale().y;
            break;
        case 3: // Plane
            jsonObj.size = obj.GetPlaneSize();
            break;
        }

        jsonObjects.push_back(jsonObj);
    }

    // Create metadata for models.json format
    MapMetadata metadata;
    metadata.version = "1.0";
    metadata.name =
        m_currentlyLoadedMapFilePath.empty() ? "exported_map" : m_currentlyLoadedMapFilePath;
    metadata.displayName     = "Exported Map";
    metadata.description     = "Map exported from ChainedDecos Map Editor";
    metadata.author          = "Map Editor";
    metadata.startPosition   = spawnPosition; // Use spawn zone position if found
    metadata.endPosition     = {0.0f, 2.0f, 0.0f};
    metadata.skyColor        = SKYBLUE;
    metadata.groundColor     = DARKGREEN;
    metadata.difficulty      = 1.0f;
    metadata.createdDate     = "2024-01-01T00:00:00Z";
    metadata.modifiedDate    = "2024-01-01T00:00:00Z";
    metadata.worldBounds     = {100.0f, 100.0f, 100.0f};
    metadata.backgroundColor = {50, 50, 50, 255};
    metadata.skyboxTexture   = m_currentMetadata.skyboxTexture;

    // Export using the new models.json format
    if (JsonMapFileManager::ExportGameMap(jsonObjects, filename, metadata))
    {
        std::cout << "Map exported for game successfully in models.json format!" << std::endl;
        std::cout << "Saved " << jsonObjects.size() << " objects" << std::endl;
        return true;
    }
    else
    {
        std::cout << "Failed to export map for game!" << std::endl;
        return false;
    }
}

bool FileManager::ExportAsJSON(const std::string &filename, const std::vector<MapObject> &objects)
{
    // Convert MapObjects to JsonSerializableObjects for JSON export
    std::vector<JsonSerializableObject> jsonObjects;

    for (const auto &obj : objects)
    {
        JsonSerializableObject jsonObj;

        jsonObj.position  = obj.GetPosition();
        jsonObj.scale     = obj.GetScale();
        jsonObj.rotation  = obj.GetRotation();
        jsonObj.color     = obj.GetColor();
        jsonObj.name      = obj.GetObjectName();
        jsonObj.type      = obj.GetObjectType();
        jsonObj.modelName = obj.GetModelAssetName();
        jsonObj.visible   = true; // Default to visible
        jsonObj.layer     = "default";
        jsonObj.tags      = "exported";
        jsonObj.id =
            "obj_" + std::to_string(rand() % 9000 + 1000) + "_" + std::to_string(time(nullptr));

        // Set shape-specific properties
        switch (obj.GetObjectType())
        {
        case 1: // Sphere
            jsonObj.radiusSphere = obj.GetSphereRadius();
            break;
        case 2: // Cylinder
            jsonObj.radiusH = obj.GetScale().x;
            jsonObj.radiusV = obj.GetScale().y;
            break;
        case 3: // Plane
            jsonObj.size = obj.GetPlaneSize();
            break;
        }

        jsonObjects.push_back(jsonObj);
    }

    // Find spawn zone and set metadata.startPosition
    Vector3 spawnPosition = {0.0f, 2.0f, 0.0f}; // Default spawn position

    for (const auto &obj : objects)
    {
        if (obj.GetObjectType() == 6) // SPAWN_ZONE
        {
            spawnPosition = obj.GetPosition();
            break;
        }
    }

    // Create metadata
    MapMetadata metadata;
    metadata.version = "1.0";
    metadata.name =
        m_currentlyLoadedMapFilePath.empty() ? "exported_map" : m_currentlyLoadedMapFilePath;
    metadata.displayName     = "Exported Map";
    metadata.description     = "Map exported from ChainedDecos Map Editor as JSON";
    metadata.author          = "Map Editor";
    metadata.startPosition   = spawnPosition; // Use spawn zone position if found
    metadata.endPosition     = {0.0f, 2.0f, 0.0f};
    metadata.skyColor        = SKYBLUE;
    metadata.groundColor     = DARKGREEN;
    metadata.difficulty      = 1.0f;
    metadata.createdDate     = "2024-01-01T00:00:00Z"; // Default timestamp
    metadata.modifiedDate    = "2024-01-01T00:00:00Z"; // Default timestamp
    metadata.worldBounds     = {100.0f, 100.0f, 100.0f};
    metadata.backgroundColor = {50, 50, 50, 255};
    metadata.skyboxTexture   = m_currentMetadata.skyboxTexture;

    // Export using JsonMapFileManager
    if (JsonMapFileManager::ExportGameMap(jsonObjects, filename, metadata))
    {
        std::cout << "Map exported as JSON successfully!" << std::endl;
        std::cout << "Saved " << jsonObjects.size() << " objects" << std::endl;
        return true;
    }
    else
    {
        std::cout << "Failed to export map as JSON!" << std::endl;
        return false;
    }
}

void FileManager::LoadParkourMap(const std::string &mapName, std::vector<MapObject> &objects)
{
    // Load the map from JSON
    MapLoader loader;
    std::string mapPath = "../resources/maps/" + mapName + ".json";
    GameMap gameMap     = loader.LoadMap(mapPath);

    // Clear current scene
    objects.clear();

    // Convert GameMap objects to MapObjects
    for (const auto &object : gameMap.GetMapObjects())
    {
        MapObject obj;

        // Set basic properties
        obj.SetPosition(object.position);
        obj.SetColor(object.color);
        obj.SetObjectName(object.name);

        // Convert MapObjectType to MapObject type
        switch (object.type)
        {
        case MapObjectType::CUBE:
            obj.SetObjectType(0); // Cube
            obj.SetScale(object.scale);
            break;
        case MapObjectType::SPHERE:
            obj.SetObjectType(1); // Sphere
            obj.SetSphereRadius(object.radius);
            break;
        case MapObjectType::CYLINDER:
            obj.SetObjectType(2); // Cylinder
            obj.SetScale(object.scale);
            break;
        case MapObjectType::PLANE:
            obj.SetObjectType(3); // Plane
            obj.SetPlaneSize(object.size);
            break;
        case MapObjectType::MODEL:
            obj.SetObjectType(5); // Model
            obj.SetModelAssetName(object.modelName);
            obj.SetScale(object.scale);
            break;
        case MapObjectType::LIGHT:
            obj.SetObjectType(0); // Use cube as approximation
            obj.SetScale(object.scale);
            break;
        }

        objects.push_back(obj);
    }

    // Restore spawn zone from metadata.startPosition if it exists
    // Check if startPosition is not at default position (0,0,0) or if it's explicitly set
    if (gameMap.GetMapMetaData().startPosition.y >= 2.0f ||
        (gameMap.GetMapMetaData().startPosition.x != 0.0f ||
         gameMap.GetMapMetaData().startPosition.y != 0.0f ||
         gameMap.GetMapMetaData().startPosition.z != 0.0f))
    {
        MapObject spawnZone;
        spawnZone.SetObjectType(6); // SPAWN_ZONE
        spawnZone.SetPosition(gameMap.GetMapMetaData().startPosition);
        spawnZone.SetObjectName("Spawn Zone");
        spawnZone.SetColor({255, 0, 255, 128}); // Magenta with transparency
        spawnZone.SetScale({2.0f, 2.0f, 2.0f}); // Default spawn zone size
        spawnZone.SetSelected(false);
        objects.push_back(spawnZone);
        TraceLog(LOG_INFO,
                 "FileManager::LoadParkourMap() - Restored spawn zone from metadata at (%.2f, "
                 "%.2f, %.2f)",
                 gameMap.GetMapMetaData().startPosition.x, gameMap.GetMapMetaData().startPosition.y,
                 gameMap.GetMapMetaData().startPosition.z);
    }

    TraceLog(LOG_INFO, "Loaded parkour map '%s' with %d elements", mapName.c_str(), objects.size());
    m_currentlyLoadedMapFilePath = mapName;
}

void FileManager::GenerateParkourMap(const std::string &mapName, std::vector<MapObject> &objects)
{
    LoadParkourMap(mapName, objects);
}

const std::vector<GameMap> &FileManager::GetAvailableParkourMaps()
{
    return m_availableParkourMaps;
}

void FileManager::ShowParkourMapSelector()
{
    // Load available parkour maps
    MapLoader loader;
    m_availableParkourMaps             = loader.LoadAllMapsFromDirectory("../resources/maps");
    m_currentlySelectedParkourMapIndex = 0;
    m_displayParkourMapDialog          = true;
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

ImVec2 FileManager::ClampWindowPosition(const ImVec2 &desiredPos, ImVec2 &windowSize)
{
    const int screenWidth  = GetScreenWidth();
    const int screenHeight = GetScreenHeight();

    // Clamp window size to fit screen
    if (windowSize.x > static_cast<float>(screenWidth))
        windowSize.x = static_cast<float>(screenWidth);
    if (windowSize.y > static_cast<float>(screenHeight))
        windowSize.y = static_cast<float>(screenHeight);

    float clampedX = desiredPos.x;
    float clampedY = desiredPos.y;

    // Clamp X position
    if (clampedX < 0.0f)
        clampedX = 0.0f;
    else if (clampedX + windowSize.x > static_cast<float>(screenWidth))
        clampedX = static_cast<float>(screenWidth) - windowSize.x;

    // Clamp Y position
    if (clampedY < 0.0f)
        clampedY = 0.0f;
    else if (clampedY + windowSize.y > static_cast<float>(screenHeight))
        clampedY = static_cast<float>(screenHeight) - windowSize.y;

    return ImVec2(clampedX, clampedY);
}

void FileManager::EnsureWindowInBounds()
{
    ImVec2 pos             = ImGui::GetWindowPos();
    ImVec2 size            = ImGui::GetWindowSize();
    const int screenWidth  = GetScreenWidth();
    const int screenHeight = GetScreenHeight();

    bool needsClamp    = false;
    ImVec2 clampedPos  = pos;
    ImVec2 clampedSize = size;

    // Clamp size
    if (clampedSize.x > static_cast<float>(screenWidth))
    {
        clampedSize.x = static_cast<float>(screenWidth);
        needsClamp    = true;
    }
    if (clampedSize.y > static_cast<float>(screenHeight))
    {
        clampedSize.y = static_cast<float>(screenHeight);
        needsClamp    = true;
    }

    // Clamp position
    if (clampedPos.x < 0.0f)
    {
        clampedPos.x = 0.0f;
        needsClamp   = true;
    }
    else if (clampedPos.x + clampedSize.x > static_cast<float>(screenWidth))
    {
        clampedPos.x = static_cast<float>(screenWidth) - clampedSize.x;
        needsClamp   = true;
    }

    if (clampedPos.y < 0.0f)
    {
        clampedPos.y = 0.0f;
        needsClamp   = true;
    }
    else if (clampedPos.y + clampedSize.y > static_cast<float>(screenHeight))
    {
        clampedPos.y = static_cast<float>(screenHeight) - clampedSize.y;
        needsClamp   = true;
    }

    // Apply clamping only if needed
    if (needsClamp)
    {
        ImGui::SetWindowPos(clampedPos, ImGuiCond_Always);
        ImGui::SetWindowSize(clampedSize, ImGuiCond_Always);
    }
}

