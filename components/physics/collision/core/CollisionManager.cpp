#include "CollisionManager.h"
#include "core/Log.h"
#include <algorithm>
#include <cfloat>
#include <future>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

bool CollisionManager::Initialize()
{
    CD_CORE_INFO("CollisionManager::Initialize() - Starting collision system initialization");

    // Collect objects that need BVH initialization
    std::vector<Collision *> bvhObjects;
    bvhObjects.reserve(m_collisionObjects.size());

    for (auto &collisionObject : m_collisionObjects)
    {
        if (collisionObject->GetCollisionType() == CollisionType::BVH_ONLY ||
            collisionObject->GetCollisionType() == CollisionType::TRIANGLE_PRECISE)
        {
            bvhObjects.push_back(collisionObject.get());
        }
    }

    CD_CORE_INFO(
        "CollisionManager::Initialize() - Found %zu objects requiring BVH initialization out "
        "of %zu total",
        bvhObjects.size(), m_collisionObjects.size());

    if (!bvhObjects.empty())
    {
        CD_CORE_INFO("CollisionManager::Initialize() - Starting parallel BVH construction...");

        std::vector<std::future<void>> futures;
        futures.reserve(bvhObjects.size());

        for (Collision *obj : bvhObjects)
        {
            futures.push_back(std::async(std::launch::async, [obj]() { obj->InitializeBVH(); }));
        }

        // Wait for all to complete
        for (auto &f : futures)
        {
            f.wait();
        }

        CD_CORE_INFO("CollisionManager::Initialize() - Parallel BVH construction complete.");
    }

    CD_CORE_INFO("CollisionManager::Initialize() - Collision system initialized with %zu collision "
                 "objects (%zu with BVH)",
                 m_collisionObjects.size(), bvhObjects.size());
    return true;
}

void CollisionManager::Shutdown()
{
    ClearColliders();
    CD_CORE_INFO("CollisionManager::Shutdown() - Collision system shutdown");
}

// Spatial partitioning optimization
void CollisionManager::UpdateSpatialPartitioning()
{
    BuildSpatialGrid(m_collisionObjects);
    BuildEntityGrid(m_entityColliders);
}

void CollisionManager::Render()
{
    // Draw all active colliders
    for (const auto &obj : m_collisionObjects)
    {
        obj->DrawDebug(GREEN);
    }

    // Also draw dynamic entity colliders
    for (const auto &pair : m_entityColliders)
    {
        pair.second->DrawDebug(SKYBLUE);
    }
}

void CollisionManager::AddCollider(std::shared_ptr<Collision> collisionObject)
{
    if (!collisionObject)
        return;

    m_collisionObjects.push_back(collisionObject);

    if (m_collisionObjects.back()->GetCollisionType() == CollisionType::BVH_ONLY ||
        m_collisionObjects.back()->GetCollisionType() == CollisionType::TRIANGLE_PRECISE)
    {
        m_collisionObjects.back()->InitializeBVH();
    }

    CD_CORE_INFO("Added collision object, total count: %zu", m_collisionObjects.size());

    // Update spatial partitioning more frequently for better performance
    if (m_collisionObjects.size() % 8 == 0) // Update every 8 objects for better accuracy
    {
        UpdateSpatialPartitioning();
    }
}

void CollisionManager::ClearColliders()
{
    m_collisionObjects.clear();
    m_staticGrid.clear();
    m_entityGrid.clear();
}

bool CollisionManager::CheckCollision(const Collision &playerCollision) const
{
    return CheckCollisionSpatial(playerCollision);
}
bool CollisionManager::CheckCollision(const Collision &playerCollision,
                                      Vector3 &collisionResponse) const
{
    if (m_collisionObjects.empty())
        return false;

    const Vector3 playerMin = playerCollision.GetMin();
    const Vector3 playerMax = playerCollision.GetMax();
    const Vector3 playerCenter = {(playerMin.x + playerMax.x) * 0.5f,
                                  (playerMin.y + playerMax.y) * 0.5f,
                                  (playerMin.z + playerMax.z) * 0.5f};
    bool collisionDetected = false;
    collisionResponse = {0, 0, 0};
    bool hasOptimalResponse = false;
    Vector3 optimalSeparationVector = {0, 0, 0};
    float optimalSeparationDistanceSquared = FLT_MAX;
    Vector3 groundSeparationVector = {0, 0, 0};
    bool hasGroundSeparationVector = false;

    // Use spatial grid
    auto objectsToCheck = GetNearbyObjectIndices(playerCollision);

    for (size_t objIndex : objectsToCheck)
    {
        if (objIndex >= m_collisionObjects.size())
            continue;
        const auto &collisionObject = m_collisionObjects[objIndex];

        if (collisionObject->IsUsingBVH())
        {
            CollisionResult res = playerCollision.CheckCollisionDetailed(*collisionObject);
            if (!res.hit)
                continue;

            collisionDetected = true;
            Vector3 mtv = res.mtv;

            // Ground check (if normal is mostly up)
            if (res.normal.y > 0.5f)
            {
                if (!hasGroundSeparationVector || fabsf(mtv.y) < fabsf(groundSeparationVector.y))
                {
                    groundSeparationVector = mtv;
                    hasGroundSeparationVector = true;
                }
            }
            else
            {
                // Sideways or ceiling collision
                float mtvLenSq = Vector3LengthSqr(mtv);
                if (!hasOptimalResponse || mtvLenSq < optimalSeparationDistanceSquared)
                {
                    optimalSeparationDistanceSquared = mtvLenSq;
                    optimalSeparationVector = mtv;
                    hasOptimalResponse = true;
                }
            }
            continue;
        }

        // Legacy/Simple AABB collision logic for non-BVH objects
        bool collisionDetectedInObject = playerCollision.Intersects(*collisionObject);
        if (!collisionDetectedInObject)
            continue;
        collisionDetected = true;

        const Vector3 collisionObjectMin = collisionObject->GetMin();
        const Vector3 collisionObjectMax = collisionObject->GetMax();
        const Vector3 collisionObjectCenter = {(collisionObjectMin.x + collisionObjectMax.x) * 0.5f,
                                               (collisionObjectMin.y + collisionObjectMax.y) * 0.5f,
                                               (collisionObjectMin.z + collisionObjectMax.z) *
                                                   0.5f};
        const float overlapX =
            fminf(playerMax.x, collisionObjectMax.x) - fmaxf(playerMin.x, collisionObjectMin.x);
        const float overlapY =
            fminf(playerMax.y, collisionObjectMax.y) - fmaxf(playerMin.y, collisionObjectMin.y);
        const float overlapZ =
            fminf(playerMax.z, collisionObjectMax.z) - fmaxf(playerMin.z, collisionObjectMin.z);
        if (overlapX <= 0 || overlapY <= 0 || overlapZ <= 0)
            continue;

        float minimumOverlap = fabsf(overlapX);
        int collisionAxis = 0;
        if (fabsf(overlapY) < minimumOverlap)
        {
            minimumOverlap = fabsf(overlapY);
            collisionAxis = 1;
        }
        if (fabsf(overlapZ) < minimumOverlap)
        {
            minimumOverlap = fabsf(overlapZ);
            collisionAxis = 2;
        }

        Vector3 mtv = {0, 0, 0};
        switch (collisionAxis)
        {
        case 0:
            mtv.x = (playerCenter.x < collisionObjectCenter.x) ? -minimumOverlap : minimumOverlap;
            break;
        case 1:
            mtv.y = (playerCenter.y < collisionObjectCenter.y) ? -minimumOverlap : minimumOverlap;
            break;
        case 2:
            mtv.z = (playerCenter.z < collisionObjectCenter.z) ? -minimumOverlap : minimumOverlap;
            break;
        }

        if (collisionAxis == 1 && mtv.y > 0)
        {
            if (!hasGroundSeparationVector || fabsf(mtv.y) < fabsf(groundSeparationVector.y))
            {
                groundSeparationVector = mtv;
                hasGroundSeparationVector = true;
            }
        }
        else
        {
            float mtvLenSq = mtv.x * mtv.x + mtv.y * mtv.y + mtv.z * mtv.z;
            if (!hasOptimalResponse || mtvLenSq < optimalSeparationDistanceSquared)
            {
                optimalSeparationDistanceSquared = mtvLenSq;
                optimalSeparationVector = mtv;
                hasOptimalResponse = true;
            }
        }
    }

    if (hasGroundSeparationVector)
    {
        collisionResponse = groundSeparationVector;
        return true;
    }
    if (hasOptimalResponse)
    {
        collisionResponse = optimalSeparationVector;
        return true;
    }

    return collisionDetected;
}

// Helper function to create cache key
std::string CollisionManager::MakeCollisionCacheKey(const std::string &modelName, float scale) const
{
    int scaledInt = static_cast<int>(roundf(scale * 1000.0f));
    return modelName + "_s" + std::to_string(scaledInt);
}

bool CollisionManager::CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                                Vector3 position, float scale,
                                                const ModelLoader &models)
{
    // Analysis and configuration
    const ModelFileConfig *config = models.GetModelConfig(modelName);
    // Default to precise for models unless very simple
    bool needsPrecise = true;

    // Cache lookup
    std::string cacheKey = MakeCollisionCacheKey(modelName, scale);
    std::shared_ptr<Collision> cached;

    auto it = m_collisionCache.find(cacheKey);
    if (it != m_collisionCache.end())
    {
        cached = it->second;
    }
    else
    {
        cached = CreateBaseCollision(model, modelName, config, needsPrecise);
        m_collisionCache[cacheKey] = cached;
    }

    // Instance creation
    Collision instance;
    if (needsPrecise &&
        m_preciseCollisionCountPerModel[modelName] < MAX_PRECISE_COLLISIONS_PER_MODEL)
    {
        if (cached->HasTriangleData())
        {
            instance = CreatePreciseInstanceCollisionFromCached(*cached, position, scale);
        }
        else
        {
            instance = CreatePreciseInstanceCollision(model, position, scale, config);
        }
        m_preciseCollisionCountPerModel[modelName]++;
    }
    else
    {
        instance = CreateSimpleAABBInstanceCollision(*cached, position, scale);
    }

    AddCollider(std::make_shared<Collision>(std::move(instance)));
    return true;
}

const std::vector<std::shared_ptr<Collision>> &CollisionManager::GetColliders() const
{
    return m_collisionObjects;
}

bool CollisionManager::RaycastDown(const Vector3 &raycastOrigin, float maxRaycastDistance,
                                   float &hitDistance, Vector3 &hitPoint, Vector3 &hitNormal) const
{
    bool anyHitDetected = false;

    float nearestHitDistance = maxRaycastDistance;
    Vector3 nearestHitPoint = {0};
    Vector3 nearestHitNormal = {0};

    Ray ray = {raycastOrigin, {0.0f, -3.0f, 0.0f}};

    for (const auto &collisionObject : m_collisionObjects)
    {
        if (collisionObject->IsUsingBVH())
        {
            RayHit raycastHit;
            raycastHit.hit = false;
            raycastHit.distance = maxRaycastDistance;
            if (collisionObject->RaycastBVH(ray, maxRaycastDistance, raycastHit))
            {
                if (raycastHit.distance < nearestHitDistance)
                {
                    nearestHitDistance = raycastHit.distance;
                    nearestHitPoint = raycastHit.position;
                    nearestHitNormal = raycastHit.normal;
                    anyHitDetected = true;
                }
            }
        }
        else
        {
            // AABB-only fallback: intersect ray with top face of AABB
            const Vector3 mn = collisionObject->GetMin();
            const Vector3 mx = collisionObject->GetMax();

            // Check if raycast origin is within or above the AABB on X and Z axes
            // Allow some tolerance for X and Z to catch nearby platforms
            const float tolerance = 2.0f; // Increase tolerance to catch nearby platforms
            if (raycastOrigin.x >= (mn.x - tolerance) && raycastOrigin.x <= (mx.x + tolerance) &&
                raycastOrigin.z >= (mn.z - tolerance) && raycastOrigin.z <= (mx.z + tolerance))
            {
                // If origin is above or at the top face, use top face as hit point
                if (raycastOrigin.y >= mx.y)
                {
                    float dist = raycastOrigin.y - mx.y;
                    if (dist <= maxRaycastDistance)
                    {
                        if (dist < nearestHitDistance)
                        {
                            nearestHitDistance = dist;
                            nearestHitPoint = {raycastOrigin.x, mx.y, raycastOrigin.z};
                            nearestHitNormal = {0, 1, 0};
                            anyHitDetected = true;
                        }
                    }
                }
                // If origin is inside the AABB (between min and max Y), use top face as hit point
                // This is important for detecting ground when player is standing on/inside the
                // collision box
                else if (raycastOrigin.y >= mn.y && raycastOrigin.y <= mx.y)
                {
                    float dist = 0.0f; // Player is inside or on top of the collision box
                    if (dist < nearestHitDistance)
                    {
                        nearestHitDistance = dist;
                        nearestHitPoint = {raycastOrigin.x, mx.y, raycastOrigin.z};
                        nearestHitNormal = {0, 1, 0};
                        anyHitDetected = true;
                    }
                }
            }
        }
    }

    if (anyHitDetected)
    {
        hitDistance = nearestHitDistance;
        hitPoint = nearestHitPoint;
        hitNormal = nearestHitNormal;
        return true;
    }
    return false;
}

// Spatial partitioning optimized collision checking
// Spatial partitioning optimized collision checking
bool CollisionManager::CheckCollisionSpatial(const Collision &playerCollision) const
{
    auto objectsToCheck = GetNearbyObjectIndices(playerCollision);
    for (size_t objIndex : objectsToCheck)
    {
        if (objIndex >= m_collisionObjects.size())
            continue;
        const auto &collisionObject = m_collisionObjects[objIndex];
        if (playerCollision.Intersects(*collisionObject))
            return true;
    }

    auto entitiesToCheck = GetNearbyEntities(playerCollision);
    for (auto entityId : entitiesToCheck)
    {
        auto colIt = m_entityColliders.find(entityId);
        if (colIt != m_entityColliders.end() && colIt->second)
        {
            if (playerCollision.Intersects(*colIt->second))
                return true;
        }
    }

    return false;
}

//
// Dynamic Entity Management
//

void CollisionManager::AddEntityCollider(ECS::EntityID entity,
                                         const std::shared_ptr<Collision> &collider)
{
    if (!collider)
        return;
    m_entityColliders[entity] = collider;
    BuildEntityGrid(m_entityColliders);
}

void CollisionManager::RemoveEntityCollider(ECS::EntityID entity)
{
    m_entityColliders.erase(entity);
    BuildEntityGrid(m_entityColliders);
}

void CollisionManager::UpdateEntityCollider(ECS::EntityID entity, const Vector3 &position)
{
    auto it = m_entityColliders.find(entity);
    if (it != m_entityColliders.end() && it->second)
    {
        Collision &col = *it->second;
        Vector3 currentSize = col.GetSize();
        // Force update position
        col.Update(position, Vector3Scale(currentSize, 0.5f));
    }
}

std::shared_ptr<Collision> CollisionManager::GetEntityCollider(ECS::EntityID entity) const
{
    auto it = m_entityColliders.find(entity);
    if (it != m_entityColliders.end())
        return it->second;
    return nullptr;
}

bool CollisionManager::CheckEntityCollision(ECS::EntityID selfEntity, const Collision &collider,
                                            std::vector<ECS::EntityID> &outCollidedEntities) const
{
    bool collisionDetected = false;
    auto entitiesToCheck = GetNearbyEntities(collider);

    for (ECS::EntityID otherEntity : entitiesToCheck)
    {
        if (otherEntity == selfEntity)
            continue;

        auto colIt = m_entityColliders.find(otherEntity);
        if (colIt != m_entityColliders.end() && colIt->second)
        {
            if (collider.Intersects(*colIt->second))
            {
                outCollidedEntities.push_back(otherEntity);
                collisionDetected = true;
            }
        }
    }

    return collisionDetected;
}

//
// Spatial Grid Implementation
//

void CollisionManager::BuildSpatialGrid(const std::vector<std::shared_ptr<Collision>> &objects)
{
    m_staticGrid.clear();
    m_staticGrid.reserve(objects.size() * 4);

    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto &obj = objects[i];
        Vector3 min = obj->GetMin();
        Vector3 max = obj->GetMax();

        int minX = static_cast<int>(floorf(min.x / m_cellSize));
        int maxX = static_cast<int>(floorf(max.x / m_cellSize));
        int minZ = static_cast<int>(floorf(min.z / m_cellSize));
        int maxZ = static_cast<int>(floorf(max.z / m_cellSize));

        for (int x = minX; x <= maxX; ++x)
        {
            for (int z = minZ; z <= maxZ; ++z)
            {
                m_staticGrid[{x, z}].push_back(i);
            }
        }
    }
}

void CollisionManager::BuildEntityGrid(
    const std::unordered_map<ECS::EntityID, std::shared_ptr<Collision>> &entityColliders)
{
    m_entityGrid.clear();
    for (const auto &[entity, collider] : entityColliders)
    {
        Vector3 min = collider->GetMin();
        Vector3 max = collider->GetMax();

        int minX = static_cast<int>(floorf(min.x / m_cellSize));
        int maxX = static_cast<int>(floorf(max.x / m_cellSize));
        int minZ = static_cast<int>(floorf(min.z / m_cellSize));
        int maxZ = static_cast<int>(floorf(max.z / m_cellSize));

        for (int x = minX; x <= maxX; ++x)
        {
            for (int z = minZ; z <= maxZ; ++z)
            {
                m_entityGrid[{x, z}].push_back(entity);
            }
        }
    }
}

std::vector<size_t> CollisionManager::GetNearbyObjectIndices(const Collision &target) const
{
    Vector3 min = target.GetMin();
    Vector3 max = target.GetMax();

    int minX = static_cast<int>(floorf(min.x / m_cellSize));
    int maxX = static_cast<int>(floorf(max.x / m_cellSize));
    int minZ = static_cast<int>(floorf(min.z / m_cellSize));
    int maxZ = static_cast<int>(floorf(max.z / m_cellSize));

    std::unordered_set<size_t> indices;
    for (int x = minX; x <= maxX; ++x)
    {
        for (int z = minZ; z <= maxZ; ++z)
        {
            GridKey key = {x, z};
            auto it = m_staticGrid.find(key);
            if (it != m_staticGrid.end())
            {
                for (size_t idx : it->second)
                {
                    indices.insert(idx);
                }
            }
        }
    }

    return std::vector<size_t>(indices.begin(), indices.end());
}

std::vector<ECS::EntityID> CollisionManager::GetNearbyEntities(const Collision &target) const
{
    Vector3 min = target.GetMin();
    Vector3 max = target.GetMax();

    int minX = static_cast<int>(floorf(min.x / m_cellSize));
    int maxX = static_cast<int>(floorf(max.x / m_cellSize));
    int minZ = static_cast<int>(floorf(min.z / m_cellSize));
    int maxZ = static_cast<int>(floorf(max.z / m_cellSize));

    std::unordered_set<ECS::EntityID> entities;
    for (int x = minX; x <= maxX; ++x)
    {
        for (int z = minZ; z <= maxZ; ++z)
        {
            GridKey key = {x, z};
            auto it = m_entityGrid.find(key);
            if (it != m_entityGrid.end())
            {
                for (auto entity : it->second)
                {
                    entities.insert(entity);
                }
            }
        }
    }

    return std::vector<ECS::EntityID>(entities.begin(), entities.end());
}

//
// Model Processor Implementation
//

std::shared_ptr<Collision> CollisionManager::CreateBaseCollision(const Model &model,
                                                                 const std::string &modelName,
                                                                 const ModelFileConfig *config,
                                                                 bool needsPreciseCollision)
{
    auto collision = std::make_shared<Collision>();

    if (model.meshCount == 0)
    {
        BoundingBox bb = GetModelBoundingBox(model);
        collision->Update(Vector3Scale(Vector3Add(bb.min, bb.max), 0.5f),
                          Vector3Scale(Vector3Subtract(bb.max, bb.min), 0.5f));
        collision->SetCollisionType(CollisionType::AABB_ONLY);
        return collision;
    }

    collision->BuildFromModel(const_cast<Model *>(&model), MatrixIdentity());

    if (needsPreciseCollision && config)
    {
        CollisionType type = CollisionType::BVH_ONLY;
        if (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE)
            type = CollisionType::TRIANGLE_PRECISE;

        collision->SetCollisionType(type);
    }
    else
    {
        collision->SetCollisionType(CollisionType::AABB_ONLY);
    }

    return collision;
}

Collision CollisionManager::CreatePreciseInstanceCollision(const Model &model, Vector3 position,
                                                           float scale,
                                                           const ModelFileConfig *config)
{
    Collision instance;
    Matrix transform = MatrixMultiply(MatrixScale(scale, scale, scale),
                                      MatrixTranslate(position.x, position.y, position.z));
    instance.BuildFromModel(const_cast<Model *>(&model), transform);
    instance.SetCollisionType(CollisionType::BVH_ONLY);
    return instance;
}

Collision
CollisionManager::CreatePreciseInstanceCollisionFromCached(const Collision &cachedCollision,
                                                           Vector3 position, float scale)
{
    Collision instance;
    Matrix transform = MatrixMultiply(MatrixScale(scale, scale, scale),
                                      MatrixTranslate(position.x, position.y, position.z));

    for (const auto &t : cachedCollision.GetTriangles())
    {
        instance.AddTriangle(CollisionTriangle(Vector3Transform(t.V0(), transform),
                                               Vector3Transform(t.V1(), transform),
                                               Vector3Transform(t.V2(), transform)));
    }

    instance.UpdateAABBFromTriangles();
    instance.InitializeBVH();
    instance.SetCollisionType(CollisionType::BVH_ONLY);
    return instance;
}

Collision CollisionManager::CreateSimpleAABBInstanceCollision(const Collision &cachedCollision,
                                                              const Vector3 &position, float scale)
{
    Vector3 center = Vector3Add(Vector3Scale(cachedCollision.GetCenter(), scale), position);
    Vector3 halfSize = Vector3Scale(cachedCollision.GetSize(), 0.5f * scale);
    Collision instance(center, halfSize);
    instance.SetCollisionType(CollisionType::AABB_ONLY);
    return instance;
}
