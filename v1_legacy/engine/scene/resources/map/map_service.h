#ifndef CD_SCENE_RESOURCES_MAP_MAP_SERVICE_H
#define CD_SCENE_RESOURCES_MAP_MAP_SERVICE_H

#include "scene_loader.h"
#include <string>

namespace CHEngine
{

class GameScene;

// Unified service for map operations used by both Editor and Game
class MapService
{
public:
    MapService() = default;
    ~MapService() = default;

    // Non-copyable, movable
    MapService(const MapService &) = delete;
    MapService &operator=(const MapService &) = delete;
    MapService(MapService &&) noexcept = default;
    MapService &operator=(MapService &&) noexcept = default;

    bool LoadScene(const std::string &filename, GameScene &gameScene);
    bool SaveScene(const std::string &filename, const GameScene &gameScene);

private:
    SceneLoader m_mapLoader;
};

} // namespace CHEngine

#endif // CD_SCENE_RESOURCES_MAP_MAP_SERVICE_H
