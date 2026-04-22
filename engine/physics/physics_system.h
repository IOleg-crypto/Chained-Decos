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
    // Ініціалізує фізичні тіла для сутностей, у яких вони ще не створені
    void InitializeBodies(Scene* scene);
    
    // Основний цикл синхронізації та симуляції
    void Update(Scene* scene, Timestep ts, bool runtime);

    // Допоміжні методи для синхронізації
    void SyncEngineToPhysics(Scene* scene);
    void SyncPhysicsToEngine(Scene* scene);

private:
    void UpdateColliders(Scene* scene);
};

} // namespace CHEngine

#endif // CH_PHYSICS_SYSTEM_H
