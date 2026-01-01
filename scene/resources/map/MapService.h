#ifndef MAPSERVICE_H
#define MAPSERVICE_H

#include "SceneLoader.h"
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

#endif // MAPSERVICE_H
