#ifndef UPDATE_MANAGER_H
#define UPDATE_MANAGER_H

#include "Engine/Collision/CollisionManager.h"
#include "MapManager.h"

class UpdateManager
{
private:
    CollisionManager* m_collisionManager;
    MapManager* m_mapManager;

public:
    UpdateManager(CollisionManager* collisionManager, MapManager* mapManager);
    ~UpdateManager() = default;

    void UpdatePhysicsLogic();
};

#endif // UPDATE_MANAGER_H

