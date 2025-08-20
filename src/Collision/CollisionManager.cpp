#include <Collision/CollisionManager.h>
#include <algorithm>
#include <raylib.h>
#include <vector>

void CollisionManager::Initialize()
{
    // Ensure all colliders are properly initialized
    for (auto &collider : m_collisions)
    {
        // Force octree initialization for complex colliders
        if (collider.GetCollisionType() == CollisionType::OCTREE_ONLY ||
            collider.GetCollisionType() == CollisionType::TRIANGLE_PRECISE ||
            collider.GetCollisionType() == CollisionType::IMPROVED_AABB)
        {
            collider.InitializeOctree();
        }
    }

    TraceLog(LOG_INFO, "CollisionManager initialized with %zu colliders", m_collisions.size());
}

void CollisionManager::AddCollider(Collision &collider)
{
    // Add a copy of the collider to our collection
    m_collisions.push_back(collider);

    // Immediately initialize octree for complex colliders
    if (collider.GetCollisionType() == CollisionType::OCTREE_ONLY ||
        collider.GetCollisionType() == CollisionType::TRIANGLE_PRECISE ||
        collider.GetCollisionType() == CollisionType::IMPROVED_AABB)
    {
        m_collisions.back().InitializeOctree();
    }

    TraceLog(LOG_INFO, "Added collider, total count: %zu", m_collisions.size());
}

void CollisionManager::ClearColliders() { m_collisions.clear(); }

bool CollisionManager::CheckCollision(const Collision &playerCollision) const
{
    // Early exit if no colliders are registered
    if (m_collisions.empty())
    {
        return false;
    }

    return std::ranges::any_of(m_collisions,
                               [&](const Collision &collider)
                               {
                                   // Use hybrid collision system (automatically chooses optimal
                                   // method)
                                   return playerCollision.Intersects(collider);
                               });
}

bool CollisionManager::CheckCollision(const Collision &playerCollision, Vector3 &response) const
{
    if (m_collisions.empty())
    {
        static int warningCounter = 0;
        if (warningCounter++ % 100 == 0)
        {
            TraceLog(LOG_WARNING, "⚠️ NO COLLIDERS FOUND! Total colliders: %zu",
                     m_collisions.size());
        }
        return false;
    }

    Vector3 playerMin = playerCollision.GetMin();
    Vector3 playerMax = playerCollision.GetMax();

    for (const auto &collider : m_collisions)
    {
        Vector3 colliderMin = collider.GetMin();
        Vector3 colliderMax = collider.GetMax();

        if (!playerCollision.Intersects(collider))
            continue;

        // Log collision detection for debugging
        Vector3 playerPos = {(playerMin.x + playerMax.x) / 2, (playerMin.y + playerMax.y) / 2,
                             (playerMin.z + playerMax.z) / 2};
        Vector3 colliderPos = {(colliderMin.x + colliderMax.x) / 2,
                               (colliderMin.y + colliderMax.y) / 2,
                               (colliderMin.z + colliderMax.z) / 2};
        float distance = Vector3Distance(playerPos, colliderPos);

        // Only log collisions when player is jumping or at significant distance
        if (fabsf(playerPos.y) > 1.0f || distance > 50.0f)
        {
            TraceLog(LOG_INFO,
                     "🔍 Collision detected at distance %.2f: Player(%.1f,%.1f,%.1f) vs "
                     "Collider(%.1f,%.1f,%.1f)",
                     distance, playerPos.x, playerPos.y, playerPos.z, colliderPos.x, colliderPos.y,
                     colliderPos.z);
        }

        bool isGroundPlane = (collider.GetCollisionType() == CollisionType::AABB_ONLY);
        bool isComplexModel = (collider.GetCollisionType() == CollisionType::OCTREE_ONLY ||
                               collider.GetCollisionType() == CollisionType::TRIANGLE_PRECISE ||
                               collider.GetCollisionType() == CollisionType::IMPROVED_AABB);

        // ------------------- MTV calculation -------------------
        Vector3 aMin = playerMin;
        Vector3 aMax = playerMax;
        Vector3 bMin = colliderMin;
        Vector3 bMax = colliderMax;

        float overlapX = fminf(aMax.x, bMax.x) - fmaxf(aMin.x, bMin.x);
        float overlapY = fminf(aMax.y, bMax.y) - fmaxf(aMin.y, bMin.y);
        float overlapZ = fminf(aMax.z, bMax.z) - fmaxf(aMin.z, bMin.z);

        if (overlapX <= 0 || overlapY <= 0 || overlapZ <= 0)
            continue;

        float dx = ((aMin.x + aMax.x) / 2 < (bMin.x + bMax.x) / 2) ? -overlapX : overlapX;
        float dy = ((aMin.y + aMax.y) / 2 < (bMin.y + bMax.y) / 2) ? -overlapY : overlapY;
        float dz = ((aMin.z + aMax.z) / 2 < (bMin.z + bMax.z) / 2) ? -overlapZ : overlapZ;

        // ------------------- Clamp for complex models -------------------
        if (isComplexModel)
        {
            const float MAX_RESPONSE = 5.0f;
            dx = fmaxf(fminf(dx, MAX_RESPONSE), -MAX_RESPONSE);
            dy = fmaxf(fminf(dy, MAX_RESPONSE), -MAX_RESPONSE);
            dz = fmaxf(fminf(dz, MAX_RESPONSE), -MAX_RESPONSE);
        }

        float absDx = fabsf(dx);
        float absDy = fabsf(dy);
        float absDz = fabsf(dz);

        // ------------------- Determine main collision axis -------------------
        if (isGroundPlane || (dy > 0.0f && absDy > absDx * 1.5f && absDy > absDz * 1.5f))
        {
            // Floor or ceiling
            response = {0, dy, 0};
        }
        else if (absDx >= absDy && absDx >= absDz)
        {
            response = {dx, 0, 0};
        }
        else
        {
            response = {0, 0, dz};
        }

        TraceLog(LOG_INFO, "📐 Final response vector: (%.3f, %.3f, %.3f) length=%.3f", response.x,
                 response.y, response.z, Vector3Length(response));

        return true;
    }

    return false;
}