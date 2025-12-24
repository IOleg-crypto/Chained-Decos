#include "Physics.h"
#include "components/physics/collision/interfaces/ICollisionManager.h"
#include "core/Engine.h"

namespace CHEngine
{

bool Physics::RaycastDown(const Vector3 &origin, float maxDistance, float &hitDistance,
                          Vector3 &hitPoint, Vector3 &hitNormal)
{
    return Engine::Instance().GetCollisionManager().RaycastDown(origin, maxDistance, hitDistance,
                                                                hitPoint, hitNormal);
}

bool Physics::CheckCollision(const ::Collision &collider)
{
    return Engine::Instance().GetCollisionManager().CheckCollision(collider);
}

bool Physics::CheckCollision(const ::Collision &collider, Vector3 &response)
{
    return Engine::Instance().GetCollisionManager().CheckCollision(collider, response);
}

void Physics::Render()
{
    Engine::Instance().GetCollisionManager().Render();
}

} // namespace CHEngine
