//
// Created by Kilo Code
//

#include "FileManager.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/MapFileManager/JsonMapFileManager.h"
#include "Engine/MapFileManager/MapFileManager.h"
#include "Game/Map/MapLoader.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iostream>
#include <misc/cpp/imgui_stdlib.h>
#include <nfd.h>
#include <raylib.h>
#include <rlImGui.h>
#include <string>

#include <raymath.h>

namespace fs = std::filesystem;

FileManager::FileManager()
    : m_displayFileDialog(false), m_isFileLoadDialog(true), m_isJsonExportDialog(false),
      m_displayNewFolderDialog(false), m_displayDeleteConfirmationDialog(false),
      m_displayParkourMapDialog(false), m_currentlySelectedParkourMapIndex(0)
{
    // Initialize file dialog to project root
    m_currentWorkingDirectory = PROJECT_ROOT_DIR;
    m_newFileNameInput = "new_map.json";
    RefreshDirectoryItems();
    // NFD init
    NFD_Init();
}

FileManager::~FileManager() { NFD_Quit(); }

bool FileManager::SaveMap(const std::string& filename, const std::vector<MapObject>& objects)
{
    // Convert MapObjects to SerializableObjects for saving
    std::vector<SerializableObject> serializableObjects;

    for (auto& obj : objects)
    {
        SerializableObject serializableObj;
        serializableObj.position = obj.GetPosition();
        serializableObj.scale = obj.GetScale();
        serializableObj.rotation = obj.GetRotation();
        serializableObj.color = obj.GetColor();
        serializableObj.name = obj.GetObjectName();
        serializableObj.type = obj.GetObjectType();
        serializableObj.modelName = obj.GetModelAssetName();
        serializableObjects.push_back(serializableObj);
    }

    // Save map to file
    if (MapFileManager::SaveMap(serializableObjects, filename))
    {
        std::cout << "Map saved successfully!" << std::endl;
        m_currentlyLoadedMapFilePath = filename;
        return true;
    }
    else
    {
        std::cout << "Failed to save map!" << std::endl;
        return false;
    }
}

bool FileManager::LoadMap(const std::string& filename, std::vector<MapObject>& objects)
{
    // Load map from file
    std::vector<SerializableObject> serializableObjects;

    if (MapFileManager::LoadMap(serializableObjects, filename))
    {
        // Clear current scene
        objects.clear();
        TraceLog(LOG_INFO , "InFO load modes");

        // Convert SerializableObjects back to MapObjects
        for (const auto& [position, scale, rotation, color, name, type, modelName] : serializableObjects)
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

        std::cout << "Map loaded successfully!" << std::endl;
        m_currentlyLoadedMapFilePath = filename;
        return true;
    }
    else
    {
        std::cout << "Failed to load map!" << std::endl;
        return false;
    }
}

bool FileManager::ExportForGame(const std::string& filename, const std::vector<MapObject>& objects)
{
    // Convert MapObjects to JsonSerializableObjects for models.json export
    std::vector<JsonSerializableObject> jsonObjects;

    for (const auto& obj : objects)
    {
        JsonSerializableObject jsonObj;

        jsonObj.position = obj.GetPosition();
        jsonObj.scale = obj.GetScale();
        jsonObj.rotation = obj.GetRotation();
        jsonObj.color = obj.GetColor();
        jsonObj.name = obj.GetObjectName();
        jsonObj.type = obj.GetObjectType();
        jsonObj.modelName = obj.GetModelAssetName();
        jsonObj.visible = true;
        jsonObj.layer = "default";
        jsonObj.tags = "exported";
        jsonObj.id = "obj_" + std::to_string(rand() % 9000 + 1000) + "_" + std::to_string(time(nullptr));

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
    metadata.name = m_currentlyLoadedMapFilePath.empty() ? "exported_map" : m_currentlyLoadedMapFilePath;
    metadata.displayName = "Exported Map";
    metadata.description = "Map exported from ChainedDecos Map Editor";
    metadata.author = "Map Editor";
    metadata.startPosition = {0.0f, 2.0f, 0.0f};
    metadata.endPosition = {0.0f, 2.0f, 0.0f};
    metadata.skyColor = SKYBLUE;
    metadata.groundColor = DARKGREEN;
    metadata.difficulty = 1.0f;
    metadata.createdDate = "2024-01-01T00:00:00Z";
    metadata.modifiedDate = "2024-01-01T00:00:00Z";
    metadata.worldBounds = {100.0f, 100.0f, 100.0f};
    metadata.backgroundColor = {50, 50, 50, 255};
    metadata.skyboxTexture = "";

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

bool FileManager::ExportAsJSON(const std::string& filename, const std::vector<MapObject>& objects)
{
    // Convert MapObjects to JsonSerializableObjects for JSON export
    std::vector<JsonSerializableObject> jsonObjects;

    for (const auto& obj : objects)
    {
        JsonSerializableObject jsonObj;

        jsonObj.position = obj.GetPosition();
        jsonObj.scale = obj.GetScale();
        jsonObj.rotation = obj.GetRotation();
        jsonObj.color = obj.GetColor();
        jsonObj.name = obj.GetObjectName();
        jsonObj.type = obj.GetObjectType();
        jsonObj.modelName = obj.GetModelAssetName();
        jsonObj.visible = true; // Default to visible
        jsonObj.layer = "default";
        jsonObj.tags = "exported";
        jsonObj.id = "obj_" + std::to_string(rand() % 9000 + 1000) + "_" + std::to_string(time(nullptr));

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

    // Create metadata
    MapMetadata metadata;
    metadata.version = "1.0";
    metadata.name = m_currentlyLoadedMapFilePath.empty() ? "exported_map" : m_currentlyLoadedMapFilePath;
    metadata.displayName = "Exported Map";
    metadata.description = "Map exported from ChainedDecos Map Editor as JSON";
    metadata.author = "Map Editor";
    metadata.startPosition = {0.0f, 2.0f, 0.0f};
    metadata.endPosition = {0.0f, 2.0f, 0.0f};
    metadata.skyColor = SKYBLUE;
    metadata.groundColor = DARKGREEN;
    metadata.difficulty = 1.0f;
    metadata.createdDate = "2024-01-01T00:00:00Z";  // Default timestamp
    metadata.modifiedDate = "2024-01-01T00:00:00Z"; // Default timestamp
    metadata.worldBounds = {100.0f, 100.0f, 100.0f};
    metadata.backgroundColor = {50, 50, 50, 255};
    metadata.skyboxTexture = "";

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

void FileManager::LoadParkourMap(const std::string& mapName, std::vector<MapObject>& objects)
{
    // Load the map from JSON
    MapLoader loader;
    std::string mapPath = "../resources/maps/" + mapName + ".json";
    GameMap gameMap = loader.LoadMap(mapPath);

    // Clear current scene
    objects.clear();

    // Convert GameMap objects to MapObjects
    for (const auto& object : gameMap.objects)
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

    TraceLog(LOG_INFO, "Loaded parkour map '%s' with %d elements", mapName.c_str(), objects.size());
    m_currentlyLoadedMapFilePath = mapName;
}

void FileManager::GenerateParkourMap(const std::string& mapName, std::vector<MapObject>& objects)
{
    LoadParkourMap(mapName, objects);
}

std::vector<GameMap> FileManager::GetAvailableParkourMaps()
{
    return m_availableParkourMaps;
}

void FileManager::ShowParkourMapSelector()
{
    // Load available parkour maps
    MapLoader loader;
    m_availableParkourMaps = loader.LoadAllMapsFromDirectory("../resources/maps");
    m_currentlySelectedParkourMapIndex = 0;
    m_displayParkourMapDialog = true;
}

void FileManager::OpenFileDialog(bool isLoad)
{
    m_isFileLoadDialog = isLoad;
    m_displayFileDialog = true;
    m_currentlySelectedFile.clear();
    m_newFileNameInput = isLoad ? "new_map.json" : "game_map.json";
    RefreshDirectoryItems();
}

void FileManager::RenderFileDialog()
{
    if (!m_displayFileDialog)
        return;

    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(GetScreenWidth() * 0.5f - 300, GetScreenHeight() * 0.5f - 200), ImGuiCond_FirstUseEver);

    std::string title = m_isFileLoadDialog ? "Load Map" : "Save Map";
    if (ImGui::Begin(title.c_str(), &m_displayFileDialog, ImGuiWindowFlags_NoCollapse))
    {
        // Current directory display
        ImGui::Text("Current Directory: %s", m_currentWorkingDirectory.c_str());
        ImGui::Separator();

        // Directory navigation
        if (ImGui::Button("Up"))
        {
            fs::path currentPath(m_currentWorkingDirectory);
            if (currentPath.has_parent_path())
            {
                NavigateToDirectory(currentPath.parent_path().string());
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
        {
            RefreshDirectoryItems();
        }
        ImGui::SameLine();
        if (ImGui::Button("New Folder"))
        {
            m_displayNewFolderDialog = true;
        }

        ImGui::Separator();

        // Directory contents
        ImGui::BeginChild("DirectoryContents", ImVec2(0, 200), true);
        for (const auto& item : m_currentDirectoryContents)
        {
            bool isDirectory = fs::is_directory(m_currentWorkingDirectory + "/" + item);
            std::string icon = isDirectory ? "[DIR]" : "[FILE]";

            if (ImGui::Selectable((icon + " " + item).c_str(), m_currentlySelectedFile == item))
            {
                m_currentlySelectedFile = item;
                if (!isDirectory)
                {
                    m_newFileNameInput = item;
                }
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (isDirectory)
                {
                    NavigateToDirectory(m_currentWorkingDirectory + "/" + item);
                }
                else if (m_isFileLoadDialog)
                {
                    // Load the file
                    std::string fullPath = m_currentWorkingDirectory + "/" + item;
                    // Note: Loading would be handled by the caller
                    m_displayFileDialog = false;
                }
            }
        }
        ImGui::EndChild();

        ImGui::Separator();

        // File name input
        ImGui::Text("File Name:");
        ImGui::InputText("##FileName", &m_newFileNameInput);

        // Action buttons
        if (ImGui::Button(m_isFileLoadDialog ? "Load" : "Save"))
        {
            if (!m_newFileNameInput.empty())
            {
                // Note: Actual save/load would be handled by the caller
                m_displayFileDialog = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            m_displayFileDialog = false;
        }
    }
    ImGui::End();

    // Render parkour map dialog if needed
    RenderParkourMapDialog();
}

void FileManager::RefreshDirectoryItems()
{
    m_currentDirectoryContents.clear();
    try
    {
        for (const auto& entry : fs::directory_iterator(m_currentWorkingDirectory))
        {
            m_currentDirectoryContents.push_back(entry.path().filename().string());
        }
        std::sort(m_currentDirectoryContents.begin(), m_currentDirectoryContents.end());
    }
    catch (const std::exception& e)
    {
        TraceLog(LOG_ERROR, "Failed to refresh directory items: %s", e.what());
    }
}

void FileManager::NavigateToDirectory(const std::string& path)
{
    if (fs::exists(path) && fs::is_directory(path))
    {
        m_currentWorkingDirectory = path;
        RefreshDirectoryItems();
    }
}

bool FileManager::IsFileDialogOpen() const
{
    return m_displayFileDialog;
}

std::string FileManager::GetCurrentWorkingDirectory() const
{
    return m_currentWorkingDirectory;
}

std::string FileManager::GetCurrentlyLoadedMapFilePath() const
{
    return m_currentlyLoadedMapFilePath;
}

void FileManager::SetCurrentlyLoadedMapFilePath(const std::string& path)
{
    m_currentlyLoadedMapFilePath = path;
}

void FileManager::RenderParkourMapDialog()
{
    if (m_displayParkourMapDialog)
    {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(GetScreenWidth() * 0.5f - 250, GetScreenHeight() * 0.5f - 200), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Parkour Maps", &m_displayParkourMapDialog, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("Select a Parkour Map:");
            ImGui::Separator();

            // List all available parkour maps
            for (int i = 0; i < m_availableParkourMaps.size(); i++)
            {
                const auto& gameMap = m_availableParkourMaps[i];

                char buffer[256];
                snprintf(buffer, sizeof(buffer), "%s (%.1f/5.0)", gameMap.metadata.displayName.c_str(), gameMap.metadata.difficulty);
                if (ImGui::Selectable(buffer, m_currentlySelectedParkourMapIndex == i))
                {
                    m_currentlySelectedParkourMapIndex = i;
                }

                // Show tooltip with description on hover
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", gameMap.metadata.description.c_str());
                    ImGui::Text("Elements: %zu", gameMap.objects.size());
                    ImGui::EndTooltip();
                }
            }

            ImGui::Separator();

            // Action buttons
            if (ImGui::Button("Load Selected Map", ImVec2(150, 30)))
            {
                if (m_currentlySelectedParkourMapIndex >= 0 && m_currentlySelectedParkourMapIndex < m_availableParkourMaps.size())
                {
                    // Note: Loading would be handled by the caller
                    m_displayParkourMapDialog = false;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 30)))
            {
                m_displayParkourMapDialog = false;
            }

            // Show selected map details
            if (m_currentlySelectedParkourMapIndex >= 0 && m_currentlySelectedParkourMapIndex < m_availableParkourMaps.size())
            {
                const auto& selectedGameMap = m_availableParkourMaps[m_currentlySelectedParkourMapIndex];
                ImGui::Separator();
                ImGui::Text("Selected Map Details:");
                ImGui::Text("Name: %s", selectedGameMap.metadata.displayName.c_str());
                ImGui::Text("Description: %s", selectedGameMap.metadata.description.c_str());
                ImGui::Text("Difficulty: %.1f/5.0", selectedGameMap.metadata.difficulty);
                ImGui::Text("Elements: %zu", selectedGameMap.objects.size());
                ImGui::Text("Start: (%.1f, %.1f, %.1f)", selectedGameMap.metadata.startPosition.x, selectedGameMap.metadata.startPosition.y, selectedGameMap.metadata.startPosition.z);
                ImGui::Text("End: (%.1f, %.1f, %.1f)", selectedGameMap.metadata.endPosition.x, selectedGameMap.metadata.endPosition.y, selectedGameMap.metadata.endPosition.z);
            }
        }
        ImGui::End();
    }
}