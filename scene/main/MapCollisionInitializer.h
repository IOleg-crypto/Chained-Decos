#ifndef MAP_COLLISION_INITIALIZER_H
#define MAP_COLLISION_INITIALIZER_H

#include "components/physics/collision/core/CollisionManager.h"
#include "core/interfaces/IPlayer.h"
#include "scene/resources/map/SceneLoader.h"
#include "scene/resources/model/Model.h"
#include <entt/entt.hpp>
#include <string>
#include <vector>

namespace CHEngine
{

// Handles collision initialization for GameScene objects
class MapCollisionInitializer
{
public:
    MapCollisionInitializer(ModelLoader *models, std::shared_ptr<::IPlayer> player = nullptr);
    ~MapCollisionInitializer() = default;

    void InitializeCollisions(entt::registry &registry, const GameScene &gameMap);
    void InitializeCollisionsWithModels(entt::registry &registry, const GameScene &gameMap,
                                        const std::vector<std::string> &requiredModels);
    bool InitializeCollisionsWithModelsSafe(entt::registry &registry, const GameScene &gameMap,
                                            const std::vector<std::string> &requiredModels);

    void SetPlayer(std::shared_ptr<::IPlayer> player);

private:
    ModelLoader *m_models;
    std::shared_ptr<::IPlayer> m_player;
};

} // namespace CHEngine

#endif // MAP_COLLISION_INITIALIZER_H
