#include "UpdateManager.h"
#include "Engine/Collision/CollisionManager.h"
#include "MapManager.h"
#include <raylib.h>

UpdateManager::UpdateManager(CollisionManager* collisionManager, MapManager* mapManager)
    : m_collisionManager(collisionManager), m_mapManager(mapManager)
{
    TraceLog(LOG_INFO, "UpdateManager created");
}

void UpdateManager::UpdatePhysicsLogic()
{
    const auto &colliders = m_collisionManager->GetColliders();
    
    if (colliders.empty())
    {
        static bool warningShown = false;
        if (!warningShown)
        {
            TraceLog(LOG_ERROR, "CRITICAL ERROR: No colliders available for physics in "
                                "UpdateManager::UpdatePhysicsLogic()!");
            warningShown = true;
        }
        
        // Create emergency ground plane if no colliders exist and no custom map is loaded
        if (m_mapManager->GetGameMap().objects.empty())
        {
            TraceLog(LOG_WARNING,
                     "UpdateManager::UpdatePhysicsLogic() - No colliders and no map objects loaded");
        }
        else
        {
            TraceLog(LOG_WARNING, "UpdateManager::UpdatePhysicsLogic() - No colliders but custom map "
                                  "loaded, using map objects for collision.");
        }
    }
    else if (colliders.size() < 2) // Only ground plane exists
    {
        static bool infoShown = false;
        if (!infoShown)
        {
            TraceLog(
                LOG_INFO,
                "UpdateManager::UpdatePhysicsLogic() - Only ground plane available, no gameplay platforms");
            infoShown = true;
        }
    }
}

