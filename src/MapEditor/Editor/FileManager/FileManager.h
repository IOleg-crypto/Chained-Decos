//
// Created by Kilo Code
//

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "IFileManager.h"
#include <memory>
#include <string>
#include <vector>
#include <imgui.h>
#include "Engine/Map/MapLoader.h"
#include "Engine/MapFileManager/MapFileManager.h"
#include "Engine/MapFileManager/JsonMapFileManager.h"

class FileManager : public IFileManager {
private:
    // Parkour map dialog state
    bool m_displayParkourMapDialog;
    std::vector<GameMap> m_availableParkourMaps;
    int m_currentlySelectedParkourMapIndex;

    // Current file path
    std::string m_currentlyLoadedMapFilePath;
    MapMetadata m_currentMetadata;

public:
    FileManager();
    ~FileManager() override;

    // File operations
    bool SaveMap(const std::string& filename, const std::vector<MapObject>& objects) override;
    bool LoadMap(const std::string& filename, std::vector<MapObject>& objects) override;
    bool ExportForGame(const std::string& filename, const std::vector<MapObject>& objects) override;
    bool ExportAsJSON(const std::string& filename, const std::vector<MapObject>& objects) override;

    // Parkour map operations
    void LoadParkourMap(const std::string& mapName, std::vector<MapObject>& objects) override;
    void GenerateParkourMap(const std::string& mapName, std::vector<MapObject>& objects) override;
    const std::vector<GameMap>& GetAvailableParkourMaps() override;
    void ShowParkourMapSelector() override;

    // Getters for dialog state
    std::string GetCurrentlyLoadedMapFilePath() const override;
    void SetCurrentlyLoadedMapFilePath(const std::string& path) override;

    const MapMetadata& GetCurrentMetadata() const override { return m_currentMetadata; }
    void SetSkyboxTexture(const std::string& path) override;

private:
    
    // Window position helper (windowSize is passed by reference to allow clamping)
    ImVec2 ClampWindowPosition(const ImVec2& desiredPos, ImVec2& windowSize);
    
    // Ensure window stays within screen bounds (call after Begin())
    void EnsureWindowInBounds();
};

#endif // FILEMANAGER_H