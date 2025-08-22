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
        return false;

    Vector3 playerMin = playerCollision.GetMin();
    Vector3 playerMax = playerCollision.GetMax();

    response = (Vector3){0, 0, 0};
    bool collided = false;

    for (const auto &collider : m_collisions)
    {
        if (!playerCollision.Intersects(collider))
            continue;

        collided = true;

        Vector3 colliderMin = collider.GetMin();
        Vector3 colliderMax = collider.GetMax();

        float overlapX = fminf(playerMax.x, colliderMax.x) - fmaxf(playerMin.x, colliderMin.x);
        float overlapY = fminf(playerMax.y, colliderMax.y) - fmaxf(playerMin.y, colliderMin.y);
        float overlapZ = fminf(playerMax.z, colliderMax.z) - fmaxf(playerMin.z, colliderMin.z);

        if (overlapX <= 0 || overlapY <= 0 || overlapZ <= 0)
            continue;

        // Обчислюємо напрям MTV
        float dx = ((playerMin.x + playerMax.x) / 2 < (colliderMin.x + colliderMax.x) / 2)
                       ? -overlapX
                       : overlapX;
        float dy = ((playerMin.y + playerMax.y) / 2 < (colliderMin.y + colliderMax.y) / 2)
                       ? -overlapY
                       : overlapY;
        float dz = ((playerMin.z + playerMax.z) / 2 < (colliderMin.z + colliderMax.z) / 2)
                       ? -overlapZ
                       : overlapZ;

        Vector3 mtv = {dx, dy, dz};
        Vector3 normal = Vector3Normalize(mtv);

        // Підлога або стеля
        if (fabsf(normal.y) > 0.7f)
        {
            mtv.x = 0; // залишаємо горизонтальну складову
            mtv.z = 0;
        }
        else
        {
            mtv.y = 0; // для стін проєктуємо вертикаль
        }

        // Пом’якшення відштовхування
        const float RESPONSE_FACTOR = 0.5f;
        mtv = Vector3Scale(mtv, RESPONSE_FACTOR);

        // Застосовуємо response послідовно
        response = Vector3Add(response, mtv);
    }

    return collided;
}