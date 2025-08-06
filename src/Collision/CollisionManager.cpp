#include <Collision/CollisionManager.h>
#include <vector>

void CollisionManager::AddCollider(Collision &collider)
{
      m_collisions.emplace_back(collider);
}

void CollisionManager::ClearColliders()
{
      m_collisions.clear();
}

bool CollisionManager::CheckCollision(const Collision &playerCollision) const
{
    for (const auto &collider : m_collisions)
    {
        if (playerCollision.Intersects(collider))
        {
            return true;
        }
    }
    return false;
}

bool CollisionManager::CheckCollision(const Collision &playerCollision, Vector3 &response) const
{
    for (const auto &collider : m_collisions)
    {
        if (playerCollision.Intersects(collider))
        {
            return true;
        }
    }
    return false;
}