//
// Created by AI Assistant
// IMapFileOperations - Interface for map file operations (load/save)
//

#ifndef IMAPFILEOPERATIONS_H
#define IMAPFILEOPERATIONS_H

#include <string>
#include <vector>
#include "MapData.h"

class IMapFileOperations
{
public:
    virtual ~IMapFileOperations() = default;
    
    virtual bool LoadMap(const std::string& filename,
                         std::vector<MapObjectData>& objects,
                         MapMetadata& metadata) = 0;
    
    virtual bool SaveMap(const std::string& filename,
                         const std::vector<MapObjectData>& objects,
                         const MapMetadata& metadata) = 0;
    
    virtual std::string GetCurrentlyLoadedMapFilePath() const = 0;
    virtual void SetCurrentlyLoadedMapFilePath(const std::string& path) = 0;
};

#endif // IMAPFILEOPERATIONS_H

