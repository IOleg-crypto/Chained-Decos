#ifndef CD_SCENE_RESOURCES_MAP_I_MAP_METADATA_MANAGER_H
#define CD_SCENE_RESOURCES_MAP_I_MAP_METADATA_MANAGER_H

#include "../core/map_data.h"
#include <string>

class IMapMetadataManager
{
public:
    virtual ~IMapMetadataManager() = default;
    
    virtual const MapMetadata& GetCurrentMetadata() const = 0;
    virtual void SetCurrentMetadata(const MapMetadata& metadata) = 0;
    virtual void SetSkyboxTexture(const std::string& path) = 0;
};

#endif // CD_SCENE_RESOURCES_MAP_I_MAP_METADATA_MANAGER_H







