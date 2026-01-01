#include "MapService.h"
#include "SceneLoader.h"

namespace CHEngine
{

bool MapService::LoadScene(const std::string &filename, GameScene &gameScene)
{
    gameScene = m_mapLoader.LoadScene(filename);
    return !gameScene.GetMapObjects().empty();
}

bool MapService::SaveScene(const std::string &filename, const GameScene &gameScene)
{
    return m_mapLoader.SaveScene(gameScene, filename);
}

} // namespace CHEngine
