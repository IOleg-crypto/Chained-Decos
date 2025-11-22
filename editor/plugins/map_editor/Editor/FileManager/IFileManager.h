//
// Created by Kilo Code
// Updated to follow Interface Segregation Principle
//

#ifndef IFILEMANAGER_H
#define IFILEMANAGER_H

#include <string>
#include <vector>
#include "../Object/MapObject.h"
#include "Engine/Map/Core/MapData.h"

class IFileManager {
public:
    virtual ~IFileManager() = default;

    virtual bool SaveMap(const std::string& filename, const std::vector<MapObject>& objects) = 0;
    virtual bool LoadMap(const std::string& filename, std::vector<MapObject>& objects) = 0;
    
    virtual std::string GetCurrentlyLoadedMapFilePath() const = 0;
    virtual void SetCurrentlyLoadedMapFilePath(const std::string& path) = 0;

    virtual const MapMetadata& GetCurrentMetadata() const = 0;
    virtual void SetSkyboxTexture(const std::string& path) = 0;
    virtual void SetCurrentMetadata(const MapMetadata& metadata) = 0;
};

#endif // IFILEMANAGER_H