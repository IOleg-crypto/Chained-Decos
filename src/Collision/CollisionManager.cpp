#include <Collision/CollisionManager.h>
#include <algorithm>
#include <raylib.h>
#include <vector>

void CollisionManager::AddCollider(Collision &collider) { m_collisions.emplace_back(collider); }

void CollisionManager::ClearColliders() { m_collisions.clear(); }

bool CollisionManager::CheckCollision(const Collision &playerCollision) const
{
    return std::any_of(m_collisions.begin(), m_collisions.end(),
                       [&](const Collision &collider)
                       {
                           // Use hybrid collision system (automatically chooses optimal method)
                           return playerCollision.Intersects(collider);
                       });
}

bool CollisionManager::CheckCollision(const Collision &playerCollision, Vector3 &response) const
{
    // Debug: Log player position for collision check
    Vector3 playerMin = playerCollision.GetMin();
    Vector3 playerMax = playerCollision.GetMax();
    // TraceLog(LOG_INFO, "🧍 Player collision check: (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f)",
    //          playerMin.x, playerMin.y, playerMin.z, playerMax.x, playerMax.y, playerMax.z);

    TraceLog(LOG_ERROR, "� DEBUG: Total colliders to check: %zu", m_collisions.size());

    int colliderIndex = 0;
    for (const auto &collider : m_collisions)
    {
        Vector3 colliderMin = collider.GetMin();
        Vector3 colliderMax = collider.GetMax();

        TraceLog(LOG_ERROR,
                 "🔍 DEBUG: Checking collider [%d]: (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f) type=%d "
                 "triangles=%zu",
                 colliderIndex, colliderMin.x, colliderMin.y, colliderMin.z, colliderMax.x,
                 colliderMax.y, colliderMax.z, (int)collider.GetCollisionType(),
                 collider.GetTriangleCount());

        // Check collision using hybrid system (automatically chooses optimal method)
        bool hasCollision = playerCollision.Intersects(collider);

        colliderIndex++;

        if (hasCollision)
        {
            // TraceLog(LOG_INFO,
            //          "🎯 COLLISION with [%d]! Player Y(%.1f to %.1f) vs Collider Y(%.1f to
            //          %.1f)", colliderIndex - 1, playerMin.y, playerMax.y, colliderMin.y,
            //          colliderMax.y);

            // For Octree/BVH colliders, we could potentially get more precise collision response
            // but for now, we'll use the standard AABB-based MTV calculation
            // TODO: Implement precise Octree-based collision response using hit points and normals

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

            // // Debug MTV calculation for non-ground colliders (ground is now last)
            // if (colliderIndex != static_cast<int>(m_collisions.size()) - 1)
            // {
            //     TraceLog(LOG_INFO, "  MTV: dx=%.3f dy=%.3f dz=%.3f (abs: %.3f %.3f %.3f)", dx,
            //     dy,
            //              dz, absDx, absDy, absDz);
            // }

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