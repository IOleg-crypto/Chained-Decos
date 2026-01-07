#ifndef CD_SCENE_RESOURCES_MAP_I_SCENE_FILE_OPERATIONS_H
#define CD_SCENE_RESOURCES_MAP_I_SCENE_FILE_OPERATIONS_H

#include "engine/scene/resources/map/core/map_data.h"

#include <string>
#include <vector>

class ISceneFileOperations
{
public:
    virtual ~ISceneFileOperations() = default;

    virtual bool LoadScene(const std::string &filename, std::vector<MapObjectData> &objects,
                           MapMetadata &metadata) = 0;

    virtual bool SaveScene(const std::string &filename, const std::vector<MapObjectData> &objects,
                           const MapMetadata &metadata) = 0;

    virtual std::string GetCurrentlyLoadedMapFilePath() const = 0;
    virtual void SetCurrentlyLoadedMapFilePath(const std::string &path) = 0;
};

#endif // CD_SCENE_RESOURCES_MAP_I_SCENE_FILE_OPERATIONS_H
