#ifndef MAPSERVICE_H
#define MAPSERVICE_H

#include "MapLoader.h"
#include <string>

// Unified service for map operations used by both Editor and Game
class MapService
{
public:
    MapService() = default;
    ~MapService() = default;
    
    // Non-copyable, movable
    MapService(const MapService&) = delete;
    MapService& operator=(const MapService&) = delete;
    MapService(MapService&&) noexcept = default;
    MapService& operator=(MapService&&) noexcept = default;
    
    bool LoadMap(const std::string& filename, GameMap& gameMap);
    bool SaveMap(const std::string& filename, const GameMap& gameMap);

private:
    MapLoader m_mapLoader;
};

#endif // MAPSERVICE_H





