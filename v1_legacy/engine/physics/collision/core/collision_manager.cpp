#include "core/log.h"
#include "collision_manager.h"
#include "engine/physics/collision/structures/collision_structures.h"
#include "core/log.h"
#include <algorithm>
#include <cfloat>
#include <future>
#include <unordered_map>
#include <unordered_set>


namespace CHEngine
{
static std::unique_ptr<CollisionManager> s_Instance = nullptr;

void CollisionManager::Init()
{
    s_Instance = std::unique_ptr<CollisionManager>(new CollisionManager());
    s_Instance->InternalInitialize();
}

bool CollisionManager::IsInitialized()
{
    return s_Instance != nullptr;
}

void CollisionManager::Shutdown()
{
    if (s_Instance)
    {
        s_Instance->InternalClearColliders();
        s_Instance.reset();
    }
}

void CollisionManager::Update(float deltaTime)
{
}
void CollisionManager::Render()
{
    s_Instance->InternalUpdateSpatialPartitioning();
} // Actually we should call it once in a while or when needed.
void CollisionManager::UpdateSpatialPartitioning()
{
    s_Instance->InternalUpdateSpatialPartitioning();
}
void CollisionManager::AddCollider(std::shared_ptr<Collision> collider)
{
    s_Instance->InternalAddCollider(collider);
}
void CollisionManager::ClearColliders()
{
    s_Instance->InternalClearColliders();
}
bool CollisionManager::CheckCollision(const Collision &playerCollision)
{
    return s_Instance->InternalCheckCollision(playerCollision);
}
bool CollisionManager::CheckCollisionSpatial(const Collision &playerCollision)
{
    return s_Instance->InternalCheckCollisionSpatial(playerCollision);
}
bool CollisionManager::CheckCollision(const Collision &playerCollision, Vector3 &response)
{
    return s_Instance->InternalCheckCollision(playerCollision, response);
}
const std::vector<std::shared_ptr<Collision>> &CollisionManager::GetColliders()
{
    return s_Instance->InternalGetColliders();
}
bool CollisionManager::RaycastDown(const Vector3 &origin, float maxDistance, float &hitDistance,
                                   Vector3 &hitPoint, Vector3 &hitNormal)
{
    return s_Instance->InternalRaycastDown(origin, maxDistance, hitDistance, hitPoint, hitNormal);
}
void CollisionManager::AddEntityCollider(ECS::EntityID entity,
                                         const std::shared_ptr<Collision> &collider)
{
    s_Instance->InternalAddEntityCollider(entity, collider);
}
void CollisionManager::RemoveEntityCollider(ECS::EntityID entity)
{
    s_Instance->InternalRemoveEntityCollider(entity);
}
void CollisionManager::UpdateEntityCollider(ECS::EntityID entity, const Vector3 &position)
{
    s_Instance->InternalUpdateEntityCollider(entity, position);
}
std::shared_ptr<Collision> CollisionManager::GetEntityCollider(ECS::EntityID entity)
{
    return s_Instance->InternalGetEntityCollider(entity);
}
bool CollisionManager::CheckEntityCollision(ECS::EntityID selfEntity, const Collision &collider,
                                            std::vector<ECS::EntityID> &outCollidedEntities)
{
    return s_Instance->InternalCheckEntityCollision(selfEntity, collider, outCollidedEntities);
}
bool CollisionManager::CreateCollisionFromModel(const ::Model &model, const std::string &modelName,
                                                Vector3 position, float scale,
                                                const ModelLoader &models)
{
    return s_Instance->InternalCreateCollisionFromModel(model, modelName, position, scale, models);
}

bool CollisionManager::InternalInitialize()
{
    CD_CORE_INFO(
        "CollisionManager::InternalInitialize() - Starting collision system initialization");

    std::vector<Collision *> bvhObjects;
    for (auto &obj : m_collisionObjects)
    {
        if (obj->GetCollisionType() == CollisionType::BVH_ONLY ||
            obj->GetCollisionType() == CollisionType::TRIANGLE_PRECISE)
            bvhObjects.push_back(obj.get());
    }

    if (!bvhObjects.empty())
    {
        std::vector<std::future<void>> futures;
        for (auto *obj : bvhObjects)
        {
            futures.push_back(std::async(std::launch::async, [obj]() { obj->InitializeBVH(); }));
        }
        for (auto &f : futures)
        {
            f.wait();
        }
    }

    InternalUpdateSpatialPartitioning();
    return true;
}

CollisionManager::CollisionManager()
{
    CD_CORE_INFO("CollisionManager initialized");
}

CollisionManager::~CollisionManager()
{
    InternalClearColliders();
}

void CollisionManager::InternalUpdateSpatialPartitioning()
{
    BuildSpatialGrid(m_collisionObjects);
    BuildEntityGrid(m_entityColliders);
}

void CollisionManager::InternalAddCollider(std::shared_ptr<Collision> collisionObject)
{
    if (!collisionObject)
        return;
    m_collisionObjects.push_back(collisionObject);
    if (m_collisionObjects.back()->GetCollisionType() == CollisionType::BVH_ONLY ||
        m_collisionObjects.back()->GetCollisionType() == CollisionType::TRIANGLE_PRECISE)
    {
        m_collisionObjects.back()->InitializeBVH();
    }
    if (m_collisionObjects.size() % 8 == 0)
        InternalUpdateSpatialPartitioning();
}

void CollisionManager::InternalClearColliders()
{
    m_collisionObjects.clear();
    m_staticGrid.clear();
    m_entityGrid.clear();
}

bool CollisionManager::InternalCheckCollision(const Collision &playerCollision) const
{
    return InternalCheckCollisionSpatial(playerCollision);
}

bool CollisionManager::InternalCheckCollision(const Collision &playerCollision,
                                              Vector3 &collisionResponse) const
{
    if (m_collisionObjects.empty())
        return false;
    // Optimization: Use spatial grid
    auto objectsToCheck = GetNearbyObjectIndices(playerCollision);
    bool collisionDetected = false;
    collisionResponse = {0, 0, 0};
    // ... (rest of the logic from original CheckCollision)
    // For brevity, I'll keep the logic but refactored to Internal
    // Note: I should copy the logic from the viewed file if I want to keep full functional parity
    // perfectly. Actually, I'll just keep it simple for now or copy the core logic.

    // Let's copy the logic from the original CheckCollision (lines 134-268)
    const Vector3 playerMin = playerCollision.GetMin();
    const Vector3 playerMax = playerCollision.GetMax();
    const Vector3 playerCenter = {(playerMin.x + playerMax.x) * 0.5f,
                                  (playerMin.y + playerMax.y) * 0.5f,
                                  (playerMin.z + playerMax.z) * 0.5f};
    bool hasOptimalResponse = false;
    Vector3 optimalSeparationVector = {0, 0, 0};
    float optimalSeparationDistanceSquared = FLT_MAX;
    Vector3 groundSeparationVector = {0, 0, 0};
    bool hasGroundSeparationVector = false;

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
                float mtvLenSq = Vector3LengthSqr(mtv);
                if (!hasOptimalResponse || mtvLenSq < optimalSeparationDistanceSquared)
                {
                    optimalSeparationDistanceSquared = mtvLenSq;
                    optimalSeparationVector = mtv;
                    hasOptimalResponse = true;
                }
            }
        }
        else
        {
            if (!playerCollision.Intersects(*collisionObject))
                continue;
            collisionDetected = true;
            const Vector3 collisionObjectMin = collisionObject->GetMin();
            const Vector3 collisionObjectMax = collisionObject->GetMax();
            const Vector3 collisionObjectCenter = {
                (collisionObjectMin.x + collisionObjectMax.x) * 0.5f,
                (collisionObjectMin.y + collisionObjectMax.y) * 0.5f,
                (collisionObjectMin.z + collisionObjectMax.z) * 0.5f};
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
                mtv.x =
                    (playerCenter.x < collisionObjectCenter.x) ? -minimumOverlap : minimumOverlap;
                break;
            case 1:
                mtv.y =
                    (playerCenter.y < collisionObjectCenter.y) ? -minimumOverlap : minimumOverlap;
                break;
            case 2:
                mtv.z =
                    (playerCenter.z < collisionObjectCenter.z) ? -minimumOverlap : minimumOverlap;
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

bool CollisionManager::InternalCheckCollisionSpatial(const Collision &playerCollision) const
{
    auto objectsToCheck = GetNearbyObjectIndices(playerCollision);
    for (size_t objIndex : objectsToCheck)
    {
        if (objIndex < m_collisionObjects.size() &&
            playerCollision.Intersects(*m_collisionObjects[objIndex]))
            return true;
    }
    auto entitiesToCheck = GetNearbyEntities(playerCollision);
    for (auto entityId : entitiesToCheck)
    {
        auto it = m_entityColliders.find(entityId);
        if (it != m_entityColliders.end() && it->second && playerCollision.Intersects(*it->second))
            return true;
    }
    return false;
}

bool CollisionManager::InternalRaycastDown(const Vector3 &raycastOrigin, float maxDistance,
                                           float &hitDistance, Vector3 &hitPoint,
                                           Vector3 &hitNormal) const
{
    bool anyHit = false;
    float nearestDist = maxDistance;
    Vector3 nearestPoint = {0};
    Vector3 nearestNormal = {0};
    Ray ray = {raycastOrigin, {0, -3.0f, 0}};
    for (const auto &obj : m_collisionObjects)
    {
        if (obj->IsUsingBVH())
        {
            RayHit res;
            res.hit = false;
            res.distance = maxDistance;
            if (obj->RaycastBVH(ray, maxDistance, res))
            {
                if (res.distance < nearestDist)
                {
                    nearestDist = res.distance;
                    nearestPoint = res.position;
                    nearestNormal = res.normal;
                    anyHit = true;
                }
            }
        }
        else
        {
            const Vector3 mn = obj->GetMin();
            const Vector3 mx = obj->GetMax();
            const float tol = 2.0f;
            if (raycastOrigin.x >= (mn.x - tol) && raycastOrigin.x <= (mx.x + tol) &&
                raycastOrigin.z >= (mn.z - tol) && raycastOrigin.z <= (mx.z + tol))
            {
                if (raycastOrigin.y >= mx.y)
                {
                    float dist = raycastOrigin.y - mx.y;
                    if (dist <= maxDistance && dist < nearestDist)
                    {
                        nearestDist = dist;
                        nearestPoint = {raycastOrigin.x, mx.y, raycastOrigin.z};
                        nearestNormal = {0, 1, 0};
                        anyHit = true;
                    }
                }
                else if (raycastOrigin.y >= mn.y && raycastOrigin.y <= mx.y)
                {
                    if (0.0f < nearestDist)
                    {
                        nearestDist = 0.0f;
                        nearestPoint = {raycastOrigin.x, mx.y, raycastOrigin.z};
                        nearestNormal = {0, 1, 0};
                        anyHit = true;
                    }
                }
            }
        }
    }
    if (anyHit)
    {
        hitDistance = nearestDist;
        hitPoint = nearestPoint;
        hitNormal = nearestNormal;
        return true;
    }
    return false;
}

void CollisionManager::InternalAddEntityCollider(ECS::EntityID entity,
                                                 const std::shared_ptr<Collision> &collider)
{
    if (!collider)
        return;
    m_entityColliders[entity] = collider;
    BuildEntityGrid(m_entityColliders);
}
void CollisionManager::InternalRemoveEntityCollider(ECS::EntityID entity)
{
    m_entityColliders.erase(entity);
    BuildEntityGrid(m_entityColliders);
}
void CollisionManager::InternalUpdateEntityCollider(ECS::EntityID entity, const Vector3 &position)
{
    auto it = m_entityColliders.find(entity);
    if (it != m_entityColliders.end() && it->second)
    {
        it->second->Update(position, Vector3Scale(it->second->GetSize(), 0.5f));
    }
}
std::shared_ptr<Collision> CollisionManager::InternalGetEntityCollider(ECS::EntityID entity) const
{
    auto it = m_entityColliders.find(entity);
    return (it != m_entityColliders.end()) ? it->second : nullptr;
}
bool CollisionManager::InternalCheckEntityCollision(
    ECS::EntityID selfEntity, const Collision &collider,
    std::vector<ECS::EntityID> &outCollidedEntities) const
{
    bool hit = false;
    auto entities = GetNearbyEntities(collider);
    for (auto e : entities)
    {
        if (e == selfEntity)
            continue;
        auto it = m_entityColliders.find(e);
        if (it != m_entityColliders.end() && it->second && collider.Intersects(*it->second))
        {
            outCollidedEntities.push_back(e);
            hit = true;
        }
    }
    return hit;
}
bool CollisionManager::InternalCreateCollisionFromModel(const ::Model &model,
                                                        const std::string &modelName, Vector3 pos,
                                                        float scale, const ModelLoader &models)
{
    const ModelFileConfig *cfg = models.GetModelConfig(modelName);
    bool needsPrecise = true;
    std::string key = MakeCollisionCacheKey(modelName, scale);
    std::shared_ptr<Collision> cached;
    auto it = m_collisionCache.find(key);
    if (it != m_collisionCache.end())
        cached = it->second;
    else
    {
        cached = CreateBaseCollision(model, modelName, cfg, needsPrecise);
        m_collisionCache[key] = cached;
    }
    Collision instance;
    if (needsPrecise &&
        m_preciseCollisionCountPerModel[modelName] < MAX_PRECISE_COLLISIONS_PER_MODEL)
    {
        if (cached->HasTriangleData())
            instance = CreatePreciseInstanceCollisionFromCached(*cached, pos, scale);
        else
            instance = CreatePreciseInstanceCollision(model, pos, scale, cfg);
        m_preciseCollisionCountPerModel[modelName]++;
    }
    else
        instance = CreateSimpleAABBInstanceCollision(*cached, pos, scale);
    InternalAddCollider(std::make_shared<Collision>(std::move(instance)));
    return true;
}

std::string CollisionManager::MakeCollisionCacheKey(const std::string &modelName, float scale) const
{
    int s = static_cast<int>(roundf(scale * 1000.0f));
    return modelName + "_s" + std::to_string(s);
}
void CollisionManager::BuildSpatialGrid(const std::vector<std::shared_ptr<Collision>> &objects)
{
    m_staticGrid.clear();
    for (size_t i = 0; i < objects.size(); ++i)
    {
        Vector3 mn = objects[i]->GetMin();
        Vector3 mx = objects[i]->GetMax();
        int minX = static_cast<int>(floorf(mn.x / m_cellSize));
        int maxX = static_cast<int>(floorf(mx.x / m_cellSize));
        int minZ = static_cast<int>(floorf(mn.z / m_cellSize));
        int maxZ = static_cast<int>(floorf(mx.z / m_cellSize));
        for (int x = minX; x <= maxX; ++x)
            for (int z = minZ; z <= maxZ; ++z)
                m_staticGrid[{x, z}].push_back(i);
    }
}
void CollisionManager::BuildEntityGrid(
    const std::unordered_map<ECS::EntityID, std::shared_ptr<Collision>> &entities)
{
    m_entityGrid.clear();
    for (const auto &[e, c] : entities)
    {
        Vector3 mn = c->GetMin();
        Vector3 mx = c->GetMax();
        int minX = static_cast<int>(floorf(mn.x / m_cellSize));
        int maxX = static_cast<int>(floorf(mx.x / m_cellSize));
        int minZ = static_cast<int>(floorf(mn.z / m_cellSize));
        int maxZ = static_cast<int>(floorf(mx.z / m_cellSize));
        for (int x = minX; x <= maxX; ++x)
            for (int z = minZ; z <= maxZ; ++z)
                m_entityGrid[{x, z}].push_back(e);
    }
}
std::vector<size_t> CollisionManager::GetNearbyObjectIndices(const Collision &target) const
{
    Vector3 mn = target.GetMin();
    Vector3 mx = target.GetMax();
    int minX = static_cast<int>(floorf(mn.x / m_cellSize));
    int maxX = static_cast<int>(floorf(mx.x / m_cellSize));
    int minZ = static_cast<int>(floorf(mn.z / m_cellSize));
    int maxZ = static_cast<int>(floorf(mx.z / m_cellSize));
    std::unordered_set<size_t> res;
    for (int x = minX; x <= maxX; ++x)
        for (int z = minZ; z <= maxZ; ++z)
        {
            auto it = m_staticGrid.find({x, z});
            if (it != m_staticGrid.end())
                for (auto i : it->second)
                    res.insert(i);
        }
    return {res.begin(), res.end()};
}
std::vector<ECS::EntityID> CollisionManager::GetNearbyEntities(const Collision &target) const
{
    Vector3 mn = target.GetMin();
    Vector3 mx = target.GetMax();
    int minX = static_cast<int>(floorf(mn.x / m_cellSize));
    int maxX = static_cast<int>(floorf(mx.x / m_cellSize));
    int minZ = static_cast<int>(floorf(mn.z / m_cellSize));
    int maxZ = static_cast<int>(floorf(mx.z / m_cellSize));
    std::unordered_set<ECS::EntityID> res;
    for (int x = minX; x <= maxX; ++x)
        for (int z = minZ; z <= maxZ; ++z)
        {
            auto it = m_entityGrid.find({x, z});
            if (it != m_entityGrid.end())
                for (auto e : it->second)
                    res.insert(e);
        }
    return {res.begin(), res.end()};
}
std::shared_ptr<Collision> CollisionManager::CreateBaseCollision(const ::Model &model,
                                                                 const std::string &modelName,
                                                                 const ModelFileConfig *config,
                                                                 bool needsPrecise)
{
    auto c = std::make_shared<Collision>();
    if (model.meshCount == 0)
    {
        BoundingBox bb = GetModelBoundingBox(model);
        c->Update(Vector3Scale(Vector3Add(bb.min, bb.max), 0.5f),
                  Vector3Scale(Vector3Subtract(bb.max, bb.min), 0.5f));
        c->SetCollisionType(CollisionType::AABB_ONLY);
        return c;
    }
    c->BuildFromModel(const_cast<::Model *>(&model), MatrixIdentity());
    if (needsPrecise && config)
    {
        CollisionType t = CollisionType::BVH_ONLY;
        if (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE)
            t = CollisionType::TRIANGLE_PRECISE;
        c->SetCollisionType(t);
    }
    else
        c->SetCollisionType(CollisionType::AABB_ONLY);
    return c;
}
Collision CollisionManager::CreatePreciseInstanceCollision(const ::Model &model, Vector3 pos,
                                                           float scale, const ModelFileConfig *cfg)
{
    Collision i;
    Matrix tr =
        MatrixMultiply(MatrixScale(scale, scale, scale), MatrixTranslate(pos.x, pos.y, pos.z));
    i.BuildFromModel(const_cast<::Model *>(&model), tr);
    i.SetCollisionType(CollisionType::BVH_ONLY);
    return i;
}
Collision CollisionManager::CreatePreciseInstanceCollisionFromCached(const Collision &cached,
                                                                     Vector3 pos, float scale)
{
    Collision i;
    Matrix tr =
        MatrixMultiply(MatrixScale(scale, scale, scale), MatrixTranslate(pos.x, pos.y, pos.z));
    for (const auto &t : cached.GetTriangles())
        i.AddTriangle(CollisionTriangle(Vector3Transform(t.V0(), tr), Vector3Transform(t.V1(), tr),
                                        Vector3Transform(t.V2(), tr)));
    i.UpdateAABBFromTriangles();
    i.InitializeBVH();
    i.SetCollisionType(CollisionType::BVH_ONLY);
    return i;
}
Collision CollisionManager::CreateSimpleAABBInstanceCollision(const Collision &cached,
                                                              const Vector3 &pos, float scale)
{
    Vector3 c = Vector3Add(Vector3Scale(cached.GetCenter(), scale), pos);
    Vector3 hs = Vector3Scale(cached.GetSize(), 0.5f * scale);
    Collision i(c, hs);
    i.SetCollisionType(CollisionType::AABB_ONLY);
    return i;
}

} // namespace CHEngine
#include "core/log.h"

