#ifndef CH_PHYSICS_SYSTEM_H
#define CH_PHYSICS_SYSTEM_H

#include "engine/core/timestep.h"
#include "engine/physics/iphysics_world.h"

namespace CHEngine
{
class Scene;

class PhysicsSystem
{
public:
    // Initializes physics bodies for entities that haven't been created yet
    void InitializeBodies(Scene* scene);
    
    // Main synchronization and simulation loop
    void Update(Scene* scene, Timestep ts, bool runtime);

    // Helper methods for synchronization
    void SyncEngineToPhysics(Scene* scene);
    void SyncPhysicsToEngine(Scene* scene);

private:
    void UpdateColliders(Scene* scene);
};

} // namespace CHEngine

#endif // CH_PHYSICS_SYSTEM_H
