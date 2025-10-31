//
// Created by Kilo Code
//

#ifndef IFILEMANAGER_H
#define IFILEMANAGER_H

#include <string>
#include <vector>
#include "../Object/MapObject.h"
#include "Engine/Map/MapLoader.h"

class IFileManager {
public:
    virtual ~IFileManager() = default;

    // File operations
    virtual bool SaveMap(const std::string& filename, const std::vector<MapObject>& objects) = 0;
    virtual bool LoadMap(const std::string& filename, std::vector<MapObject>& objects) = 0;
    virtual bool ExportForGame(const std::string& filename, const std::vector<MapObject>& objects) = 0;
    virtual bool ExportAsJSON(const std::string& filename, const std::vector<MapObject>& objects) = 0;

    // Parkour map operations
    virtual void LoadParkourMap(const std::string& mapName, std::vector<MapObject>& objects) = 0;
    virtual void GenerateParkourMap(const std::string& mapName, std::vector<MapObject>& objects) = 0;
    virtual std::vector<GameMap> GetAvailableParkourMaps() = 0;
    virtual void ShowParkourMapSelector() = 0;

    // File dialog operations
    virtual void OpenFileDialog(bool isLoad) = 0;
    virtual void RenderFileDialog() = 0;
    virtual void RefreshDirectoryItems() = 0;
    virtual void NavigateToDirectory(const std::string& path) = 0;

    // Getters for dialog state
    virtual bool IsFileDialogOpen() const = 0;
    virtual std::string GetCurrentWorkingDirectory() const = 0;
    virtual std::string GetCurrentlyLoadedMapFilePath() const = 0;
    virtual void SetCurrentlyLoadedMapFilePath(const std::string& path) = 0;
};

#endif // IFILEMANAGER_H