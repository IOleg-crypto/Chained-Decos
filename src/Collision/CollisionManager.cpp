#include <algorithm>
#include <vector>
#include <Collision/CollisionManager.h>

void CollisionManager::AddCollider(Collision &collider) { m_collisions.emplace_back(collider); }

void CollisionManager::ClearColliders() { m_collisions.clear(); }

bool CollisionManager::CheckCollision(const Collision &playerCollision) const
{
    return std::ranges::any_of(m_collisions, [&](const Collision& collider) {
        return playerCollision.Intersects(collider);
    });
}

bool CollisionManager::CheckCollision(const Collision &playerCollision, Vector3 &response) const
{
    for (const auto &collider : m_collisions)
    {
        if (playerCollision.Intersects(collider))
        {
            // Calculate the response vector for collision resolution - Minimum Translation Vector
            // (MTV)
            Vector3 aMin = playerCollision.GetMin();
            Vector3 aMax = playerCollision.GetMax();
            Vector3 bMin = collider.GetMin();
            Vector3 bMax = collider.GetMax();

            float dx1 = bMax.x - aMin.x;
            float dx2 = aMax.x - bMin.x;
            float dy1 = bMax.y - aMin.y;
            float dy2 = aMax.y - bMin.y;
            float dz1 = bMax.z - aMin.z;
            float dz2 = aMax.z - bMin.z;

            float dx = (dx1 < dx2) ? dx1 : -dx2;
            float dy = (dy1 < dy2) ? dy1 : -dy2;
            float dz = (dz1 < dz2) ? dz1 : -dz2;
            float absDx = fabsf(dx);
            float absDy = fabsf(dy);
            float absDz = fabsf(dz);

            if (absDx < absDy && absDx < absDz)
                response = {dx, 0, 0};
            else if (absDy < absDz)
                response = {0, dy, 0};
            else
                response = {0, 0, dz};
            return true;
        }
    }
    return false;
}