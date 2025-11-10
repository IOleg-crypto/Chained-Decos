//
// Created by Kilo Code
// Refactored to use MapService and follow SOLID principles
//

#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "IFileManager.h"
#include <string>
#include <vector>
#include "Engine/Map/MapService.h"
#include "Engine/Map/MapData.h"

class FileManager : public IFileManager {
private:
    std::string m_currentlyLoadedMapFilePath;
    MapMetadata m_currentMetadata;
    MapService m_mapService;

public:
    FileManager();
    ~FileManager() override;

    bool SaveMap(const std::string& filename, const std::vector<MapObject>& objects) override;
    bool LoadMap(const std::string& filename, std::vector<MapObject>& objects) override;

    std::string GetCurrentlyLoadedMapFilePath() const override;
    void SetCurrentlyLoadedMapFilePath(const std::string& path) override;

    const MapMetadata& GetCurrentMetadata() const override { return m_currentMetadata; }
    void SetSkyboxTexture(const std::string& path) override;
    void SetCurrentMetadata(const MapMetadata& metadata);
};

#endif // FILEMANAGER_H