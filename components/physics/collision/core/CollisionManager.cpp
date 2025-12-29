#include "CollisionManager.h"
#include "core/Log.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <components/physics/collision/colsystem/CollisionSystem.h>
#include <future>
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

bool CollisionManager::Initialize()
{
    CD_CORE_INFO("CollisionManager::Initialize() - Starting collision system initialization");

    // Batch BVH initialization for better performance
    std::vector<Collision *> bvhObjects;
    bvhObjects.reserve(m_collisionObjects.size());

    // Collect objects that need BVH initialization
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

    // Initialize BVH for collected objects in parallel if multiple
    if (bvhObjects.size() > 1)
    {
// Use parallel execution if available (C++17 and supported platform)
#if defined(__cpp_lib_execution) && (__cpp_lib_execution >= 201603)
        try
        {
            std::for_each(std::execution::par, bvhObjects.begin(), bvhObjects.end(),
                          [](Collision *obj) { obj->InitializeBVH(); });
            CD_CORE_INFO("CollisionManager::Initialize() - BVH initialization completed "
                         "using parallel execution for %zu objects",
                         bvhObjects.size());
        }
        catch (const std::exception &e)
        {
            // Fallback to sequential if parallel execution fails
            CD_CORE_WARN(
                "CollisionManager::Initialize() - Parallel BVH initialization failed, falling "
                "back to sequential: %s",
                e.what());
            for (Collision *obj : bvhObjects)
            {
                obj->InitializeBVH();
            }
        }
#else
        // Fallback for platforms without std::execution support
        CD_CORE_INFO("CollisionManager::Initialize() - Using sequential BVH initialization "
                     "(parallel execution not supported or enabled on this platform)");
        for (Collision *obj : bvhObjects)
        {
            obj->InitializeBVH();
        }
#endif
    }
    else if (bvhObjects.size() == 1)
    {
        TraceLog(
            LOG_INFO,
            "CollisionManager::Initialize() - Using sequential BVH initialization for 1 object");
        bvhObjects[0]->InitializeBVH();
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
    m_grid.Build(m_collisionObjects);
    m_grid.BuildEntities(m_entityColliders);
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
    m_grid.Clear();
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

    // Check prediction cache first
    bool hit = false;
    if (const_cast<CollisionManager *>(this)->m_cache.TryGet(playerCollision, m_currentFrame,
                                                             collisionResponse, hit))
    {
        return hit;
    }

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
    auto objectsToCheck = m_grid.GetNearbyObjectIndices(playerCollision);

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
        const_cast<CollisionManager *>(this)->m_cache.Set(playerCollision, m_currentFrame, true,
                                                          collisionResponse);
        return true;
    }
    if (hasOptimalResponse)
    {
        collisionResponse = optimalSeparationVector;
        const_cast<CollisionManager *>(this)->m_cache.Set(playerCollision, m_currentFrame, true,
                                                          collisionResponse);
        return true;
    }

    const_cast<CollisionManager *>(this)->m_cache.Set(playerCollision, m_currentFrame,
                                                      collisionDetected, collisionResponse);
    return collisionDetected;
}

void CollisionManager::CreateAutoCollisionsFromModelsSelective(
    ModelLoader &models, const std::vector<std::string> &modelNames)
{
    constexpr size_t MAX_COLLISION_INSTANCES = 1000;
    int collisionObjectsCreated = 0;

    CD_CORE_INFO("Starting selective automatic collision generation for %zu specified models...",
                 modelNames.size());

    // Prevent excessive collision creation that could cause memory issues
    if (modelNames.size() > MAX_COLLISION_INSTANCES)
    {
        CD_CORE_ERROR(
            "CollisionManager::CreateAutoCollisionsFromModelsSelective() - Too many models "
            "(%zu), limiting to %zu",
            modelNames.size(), MAX_COLLISION_INSTANCES);
        return;
    }

    // Create a set for faster lookup
    std::set<std::string> modelSet(modelNames.begin(), modelNames.end());

    // Get all available models
    auto availableModels = models.GetAvailableModels();
    CD_CORE_INFO("Found %zu models available, filtering to %zu specified models",
                 availableModels.size(), modelNames.size());

    // Track processed models to avoid duplication
    std::set<std::string> processedModelNames;

    std::vector<ModelCollisionTask> tasks;

    // Prepare tasks for parallel processing (only for specified models)
    for (const auto &modelName : availableModels)
    {
        // Skip models not in our selective list
        if (modelSet.find(modelName) == modelSet.end())
        {
            CD_CORE_TRACE("Skipping collision creation for model '%s' (not in selective list)",
                          modelName.c_str());
            continue;
        }

        if (processedModelNames.find(modelName) != processedModelNames.end())
            continue;

        processedModelNames.insert(modelName);

        auto modelOpt = models.GetModelByName(modelName);
        if (!modelOpt)
        {
            CD_CORE_WARN("CollisionManager - Model not found: %s", modelName.c_str());
            continue;
        }

        Model &model = modelOpt->get();
        bool hasCollision = models.HasCollision(modelName);

        if (!hasCollision || model.meshCount == 0)
            continue;

        ModelCollisionTask task;
        task.modelName = modelName;
        task.model = &model;
        task.hasCollision = hasCollision;
        task.instances = models.GetInstancesByTag(modelName);
        tasks.push_back(task);
    }

    // Process models in parallel
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 1; // Fallback for systems that return 0
    numThreads = std::min(tasks.size(), numThreads);

    if (numThreads == 0 || tasks.empty())
    {
        CD_CORE_WARN(
            "No tasks to process or no threads available for parallel collision generation");
        return;
    }

    std::vector<std::future<int>> futures;

    // Split tasks into chunks for parallel processing
    size_t chunkSize = tasks.size() / numThreads;
    if (chunkSize == 0)
        chunkSize = 1;

    for (size_t threadIdx = 0; threadIdx < numThreads; ++threadIdx)
    {
        size_t startIdx = threadIdx * chunkSize;
        size_t endIdx = (threadIdx == numThreads - 1) ? tasks.size() : (threadIdx + 1) * chunkSize;

        if (startIdx >= tasks.size())
            break;

        futures.push_back(std::async(
            std::launch::async,
            [this, &models, &tasks, startIdx, endIdx, MAX_COLLISION_INSTANCES]()
            {
                int localCollisionsCreated = 0;

                for (size_t i = startIdx; i < endIdx; ++i)
                {
                    const auto &task = tasks[i];

                    CD_CORE_INFO("Processing selective model: %s", task.modelName.c_str());

                    if (task.instances.empty())
                    {
                        // No instances found, create default collision
                        Vector3 defaultPos =
                            (task.modelName == "arc") ? Vector3{0, 0, 140} : Vector3{0, 0, 0};
                        if (CreateCollisionFromModel(*task.model, task.modelName, defaultPos, 1.0f,
                                                     models))
                        {
                            localCollisionsCreated++;
                        }
                    }
                    else
                    {
                        // Create collisions for each instance (up to the limit)
                        size_t instanceLimit =
                            std::min(task.instances.size(), MAX_COLLISION_INSTANCES);
                        CD_CORE_INFO("Processing %zu/%zu instances for selective model '%s'",
                                     instanceLimit, task.instances.size(), task.modelName.c_str());

                        for (size_t j = 0; j < instanceLimit; j++)
                        {
                            auto *instance = task.instances[j];
                            Vector3 position = instance->GetModelPosition();
                            float scale = instance->GetScale();

                            if (CreateCollisionFromModel(*task.model, task.modelName, position,
                                                         scale, models))
                            {
                                localCollisionsCreated++;
                            }
                        }

                        if (task.instances.size() > MAX_COLLISION_INSTANCES)
                        {
                            CD_CORE_WARN(
                                "Limited collisions for selective model '%s' to %zu (of %zu "
                                "instances)",
                                task.modelName.c_str(), MAX_COLLISION_INSTANCES,
                                task.instances.size());
                        }
                    }
                }

                return localCollisionsCreated;
            }));
    }

    // Collect results from all threads
    for (auto &future : futures)
    {
        collisionObjectsCreated += future.get();
    }

    CD_CORE_INFO(
        "Selective automatic collision generation complete. Created %d collision objects from "
        "%zu specified models",
        collisionObjectsCreated, modelNames.size());

    // Final spatial partitioning update for optimal performance
    UpdateSpatialPartitioning();
}

// Helper function to create cache key
std::string CollisionManager::MakeCollisionCacheKey(const std::string &modelName, float scale) const
{
    return m_modelProcessor.MakeCollisionCacheKey(modelName, scale);
}

bool CollisionManager::CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                                Vector3 position, float scale,
                                                const ModelLoader &models)
{
    // Analysis and configuration
    const ModelFileConfig *config = models.GetModelConfig(modelName);
    bool needsPrecise = m_modelProcessor.AnalyzeModelShape(model, modelName);

    // Cache lookup
    std::string cacheKey = m_modelProcessor.MakeCollisionCacheKey(modelName, scale);
    std::shared_ptr<Collision> cached;

    auto it = m_collisionCache.find(cacheKey);
    if (it != m_collisionCache.end())
    {
        cached = it->second;
    }
    else
    {
        cached = m_modelProcessor.CreateBaseCollision(model, modelName, config, needsPrecise);
        m_collisionCache[cacheKey] = cached;
    }

    // Instance creation
    Collision instance;
    if (needsPrecise &&
        m_preciseCollisionCountPerModel[modelName] < MAX_PRECISE_COLLISIONS_PER_MODEL)
    {
        if (cached->HasTriangleData())
        {
            instance =
                m_modelProcessor.CreatePreciseInstanceCollisionFromCached(*cached, position, scale);
        }
        else
        {
            instance =
                m_modelProcessor.CreatePreciseInstanceCollision(model, position, scale, config);
        }
        m_preciseCollisionCountPerModel[modelName]++;
    }
    else
    {
        instance = m_modelProcessor.CreateSimpleAABBInstanceCollision(*cached, position, scale);
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

    Vector3 raycastDirection = {0.0f, -3.0f, 0.0f};

    for (const auto &collisionObject : m_collisionObjects)
    {
        if (collisionObject->IsUsingBVH())
        {
            RayHit raycastHit;
            raycastHit.hit = false;
            raycastHit.distance = maxRaycastDistance;
            if (collisionObject->RaycastBVH(raycastOrigin, raycastDirection, maxRaycastDistance,
                                            raycastHit))
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
    auto objectsToCheck = m_grid.GetNearbyObjectIndices(playerCollision);
    for (size_t objIndex : objectsToCheck)
    {
        if (objIndex >= m_collisionObjects.size())
            continue;
        const auto &collisionObject = m_collisionObjects[objIndex];
        if (playerCollision.Intersects(*collisionObject))
            return true;
    }

    auto entitiesToCheck = m_grid.GetNearbyEntities(playerCollision);
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

// Prediction cache management
void CollisionManager::UpdateFrameCache()
{
    m_currentFrame++;
    m_cache.Update(m_currentFrame);
}

void CollisionManager::ClearExpiredCache()
{
    m_cache.Clear();
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
    m_grid.BuildEntities(m_entityColliders);
}

void CollisionManager::RemoveEntityCollider(ECS::EntityID entity)
{
    m_entityColliders.erase(entity);
    m_grid.BuildEntities(m_entityColliders);
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
    auto entitiesToCheck = m_grid.GetNearbyEntities(collider);

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
