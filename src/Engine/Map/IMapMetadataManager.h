//
// Created by AI Assistant
// IMapMetadataManager - Interface for map metadata management
//

#ifndef IMAPMETADATAMANAGER_H
#define IMAPMETADATAMANAGER_H

#include "MapData.h"
#include <string>

class IMapMetadataManager
{
public:
    virtual ~IMapMetadataManager() = default;
    
    virtual const MapMetadata& GetCurrentMetadata() const = 0;
    virtual void SetCurrentMetadata(const MapMetadata& metadata) = 0;
    virtual void SetSkyboxTexture(const std::string& path) = 0;
};

#endif // IMAPMETADATAMANAGER_H

