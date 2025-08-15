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
                           // Use BVH if available, otherwise fallback to AABB
                           if (collider.IsUsingBVH())
                           {
                               return collider.IntersectsBVH(playerCollision);
                           }
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

    // TraceLog(LOG_INFO, "📋 Total colliders to check: %zu", m_collisions.size());

    int colliderIndex = 0;
    for (const auto &collider : m_collisions)
    {
        Vector3 colliderMin = collider.GetMin();
        Vector3 colliderMax = collider.GetMax();

        // Check collision using appropriate method
        bool hasCollision = false;

        // if (collider.IsUsingBVH())
        // {
        //     TraceLog(LOG_INFO,
        //              "🔍 [%d] Checking BVH collider (Arena): (%.2f,%.2f,%.2f) to
        //              (%.2f,%.2f,%.2f)", colliderIndex, colliderMin.x, colliderMin.y,
        //              colliderMin.z, colliderMax.x, colliderMax.y, colliderMax.z);
        //     hasCollision = collider.IntersectsBVH(playerCollision);
        // }
        // else
        // {
        //     TraceLog(
        //         LOG_INFO,
        //         "🔍 [%d] Checking AABB collider (Ground): (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f)",
        //         colliderIndex, colliderMin.x, colliderMin.y, colliderMin.z, colliderMax.x,
        //         colliderMax.y, colliderMax.z);
        //     hasCollision = playerCollision.Intersects(collider);
        // }

        colliderIndex++;

        if (hasCollision)
        {
            // For BVH colliders, we could potentially get more precise collision response
            // but for now, we'll use the standard AABB-based MTV calculation
            // TODO: Implement precise BVH-based collision response using hit points and normals

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

            // // Log collision type for debugging
            // if (collider.IsUsingBVH())
            // {
            //     TraceLog(LOG_INFO,
            //              "🎯 [%d] BVH (Arena) Collision WINNER! Response: (%.3f, %.3f, %.3f)",
            //              colliderIndex - 1, response.x, response.y, response.z);
            // }
            // else
            // {
            //     TraceLog(LOG_INFO,
            //              "📦 [%d] AABB (Ground) Collision WINNER! Response: (%.3f, %.3f, %.3f)",
            //              colliderIndex - 1, response.x, response.y, response.z);
            // }

            return true;
        }
    }
    return false;
}