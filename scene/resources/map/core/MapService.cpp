#include "MapService.h"
#include "MapLoader.h"

bool MapService::LoadMap(const std::string& filename, GameMap& gameMap)
{
    gameMap = m_mapLoader.LoadMap(filename);
    return !gameMap.GetMapObjects().empty();
}

bool MapService::SaveMap(const std::string& filename, const GameMap& gameMap)
{
    return m_mapLoader.SaveMap(gameMap, filename);
}





