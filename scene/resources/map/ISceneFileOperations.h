#ifndef ISCENEFILEOPERATIONS_H
#define ISCENEFILEOPERATIONS_H

#include "scene/resources/map/core/MapData.h"

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

#endif // ISCENEFILEOPERATIONS_H
