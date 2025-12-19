#include "CollisionManager.h"
#include <algorithm>
#include <array>
#include <cfloat>
#include <compare>
#include <components/physics/collision/System/CollisionSystem.h>
#include <execution>
#include <future>
#include <raylib.h>
#include <raymath.h>
#include <scene/resources/model/Core/Model.h>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

bool CollisionManager::Initialize()
{
    TraceLog(LOG_INFO, "CollisionManager::Initialize() - Starting collision system initialization");

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

    TraceLog(LOG_INFO,
             "CollisionManager::Initialize() - Found %zu objects requiring BVH initialization out "
             "of %zu total",
             bvhObjects.size(), m_collisionObjects.size());

    // Initialize BVH for collected objects in parallel if many
    if (bvhObjects.size() > 8)
    {
// Use parallel execution if available (C++17 and supported platform)
#if defined(__cpp_lib_execution) && (__cpp_lib_execution >= 201603)
        try
        {
            std::for_each(std::execution::par, bvhObjects.begin(), bvhObjects.end(),
                          [](Collision *obj) { obj->InitializeBVH(); });
            TraceLog(LOG_INFO, "CollisionManager::Initialize() - BVH initialization completed "
                               "using parallel execution");
        }
        catch (const std::exception &e)
        {
            // Fallback to sequential if parallel execution fails
            TraceLog(LOG_WARNING,
                     "CollisionManager::Initialize() - Parallel BVH initialization failed, falling "
                     "back to sequential: %s",
                     e.what());
            for (Collision *obj : bvhObjects)
            {
                obj->InitializeBVH();
            }
        }
#else
        // Fallback for platforms without std::execution support (older macOS, etc.)
        TraceLog(LOG_INFO, "CollisionManager::Initialize() - Using sequential BVH initialization "
                           "(parallel execution not supported on this platform)");
        for (Collision *obj : bvhObjects)
        {
            obj->InitializeBVH();
        }
#endif
    }
    else
    {
        TraceLog(
            LOG_INFO,
            "CollisionManager::Initialize() - Using sequential BVH initialization for %zu objects",
            bvhObjects.size());
        for (Collision *obj : bvhObjects)
        {
            obj->InitializeBVH();
        }
    }

    TraceLog(LOG_INFO,
             "CollisionManager::Initialize() - Collision system initialized with %zu collision "
             "objects (%zu with BVH)",
             m_collisionObjects.size(), bvhObjects.size());
    return true;
}

void CollisionManager::Shutdown()
{
    ClearColliders();
    TraceLog(LOG_INFO, "CollisionManager::Shutdown() - Collision system shutdown");
}

// Spatial partitioning optimization
void CollisionManager::UpdateSpatialPartitioning()
{
    if (m_collisionObjects.empty())
        return;

    // Reserve space for better performance
    m_spatialGrid.clear();
    m_spatialGrid.reserve(m_collisionObjects.size() * 4); // Estimate 4 cells per object

    // Grid cell size (adjust based on typical object sizes)
    const float cellSize = 10.0f;

    for (size_t i = 0; i < m_collisionObjects.size(); ++i)
    {
        const auto &collisionObject = m_collisionObjects[i];
        Vector3 min = collisionObject->GetMin();
        Vector3 max = collisionObject->GetMax();

        // Calculate grid cells this object spans
        int minX = static_cast<int>(floorf(min.x / cellSize));
        int maxX = static_cast<int>(floorf(max.x / cellSize));
        int minZ = static_cast<int>(floorf(min.z / cellSize));
        int maxZ = static_cast<int>(floorf(max.z / cellSize));

        // Add object to all cells it spans (optimized loop)
        for (int x = minX; x <= maxX; ++x)
        {
            for (int z = minZ; z <= maxZ; ++z)
            {
                GridKey key = {x, z};
                m_spatialGrid[key].push_back(i);
            }
        }
    }

    TraceLog(LOG_DEBUG, "Updated spatial partitioning: %zu cells created for %zu objects",
             m_spatialGrid.size(), m_collisionObjects.size());
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

    TraceLog(LOG_INFO, "Added collision object, total count: %zu", m_collisionObjects.size());

    // Update spatial partitioning more frequently for better performance
    if (m_collisionObjects.size() % 8 == 0) // Update every 8 objects for better accuracy
    {
        UpdateSpatialPartitioning();
    }
}

void CollisionManager::ClearColliders()
{
    m_collisionObjects.clear();
}

bool CollisionManager::CheckCollision(const Collision &playerCollision) const
{
    if (m_collisionObjects.empty())
        return false;

    // Use spatial partitioning for faster collision queries
    if (!m_spatialGrid.empty())
    {
        return CheckCollisionSpatial(playerCollision);
    }

    // Fallback to original method if spatial partitioning not available
    for (const auto &collisionObject : m_collisionObjects)
    {
        bool collisionDetected = false;

        if (collisionObject->IsUsingBVH())
            collisionDetected = playerCollision.IntersectsBVH(*collisionObject);
        else
            collisionDetected = playerCollision.Intersects(*collisionObject);

        if (collisionDetected)
            return true;
    }
    return false;
}
bool CollisionManager::CheckCollision(const Collision &playerCollision,
                                      Vector3 &collisionResponse) const
{
    if (m_collisionObjects.empty())
        return false;

    // Check prediction cache first
    size_t cacheHash =
        const_cast<CollisionManager *>(this)->GetPredictionCacheHash(playerCollision);
    auto cacheIt = m_predictionCache.find(cacheHash);
    if (cacheIt != m_predictionCache.end() &&
        m_currentFrame - cacheIt->second.frameCount < CACHE_LIFETIME_FRAMES)
    {
        collisionResponse = cacheIt->second.response;
        return cacheIt->second.hasCollision;
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
    for (const auto &collisionObject : m_collisionObjects)
    {
        bool collisionDetectedInObject = collisionObject->IsUsingBVH()
                                             ? playerCollision.IntersectsBVH(*collisionObject)
                                             : playerCollision.Intersects(*collisionObject);
        if (!collisionDetectedInObject)
            continue;
        collisionDetected = true;

        if (collisionObject->IsUsingBVH())
        {
            Vector3 raycastOrigin = playerCenter;
            Vector3 raycastDirection = {0.0f, -1.0f, 0.0f};
            float maxRaycastDistance = (playerMax.y - playerMin.y) + 1.0f;
            RayHit raycastHit;
            if (collisionObject->RaycastBVH(raycastOrigin, raycastDirection, maxRaycastDistance,
                                            raycastHit) &&
                raycastHit.hit)
            {
                float upwardDelta = raycastHit.position.y - playerMin.y;
                if (upwardDelta > 0.0f && upwardDelta < maxRaycastDistance)
                {
                    Vector3 refinedSeparationVector = {0.0f, upwardDelta, 0.0f};
                    if (!hasGroundSeparationVector ||
                        fabsf(refinedSeparationVector.y) < fabsf(groundSeparationVector.y))
                    {
                        groundSeparationVector = refinedSeparationVector;
                        hasGroundSeparationVector = true;
                    }
                }
            }
        }
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
        Vector3 minimumTranslationVector = {0, 0, 0};
        switch (collisionAxis)
        {
        case 0:
            minimumTranslationVector.x =
                (playerCenter.x < collisionObjectCenter.x) ? -minimumOverlap : minimumOverlap;
            break;
        case 1:
            minimumTranslationVector.y =
                (playerCenter.y < collisionObjectCenter.y) ? -minimumOverlap : minimumOverlap;
            break;
        case 2:
            minimumTranslationVector.z =
                (playerCenter.z < collisionObjectCenter.z) ? -minimumOverlap : minimumOverlap;
            break;
        default:
            break;
        }

        // For BVH colliders: align MTV with triangle normal to better handle slopes/uneven surfaces
        if (collisionObject->IsUsingBVH())
        {
            Vector3 surfaceNormalDirection = minimumTranslationVector;
            float surfaceNormalDirectionLength =
                sqrtf(surfaceNormalDirection.x * surfaceNormalDirection.x +
                      surfaceNormalDirection.y * surfaceNormalDirection.y +
                      surfaceNormalDirection.z * surfaceNormalDirection.z);
            if (surfaceNormalDirectionLength > 1e-5f)
            {
                surfaceNormalDirection.x /= surfaceNormalDirectionLength;
                surfaceNormalDirection.y /= surfaceNormalDirectionLength;
                surfaceNormalDirection.z /= surfaceNormalDirectionLength;
                // Cast from player center opposite to MTV to find nearest contact and normal
                RayHit normalHit;
                normalHit.hit = false;
                if (collisionObject->RaycastBVH(
                        playerCenter,
                        {-surfaceNormalDirection.x, -surfaceNormalDirection.y,
                         -surfaceNormalDirection.z},
                        fminf(surfaceNormalDirectionLength + 0.5f, 2.0f), normalHit) &&
                    normalHit.hit)
                {
                    Vector3 surfaceNormal = normalHit.normal;
                    float normalDotTranslationVector =
                        surfaceNormal.x * minimumTranslationVector.x +
                        surfaceNormal.y * minimumTranslationVector.y +
                        surfaceNormal.z * minimumTranslationVector.z;
                    if (normalDotTranslationVector > 0.0f)
                    {
                        // Project MTV onto the surface normal
                        minimumTranslationVector.x = surfaceNormal.x * normalDotTranslationVector;
                        minimumTranslationVector.y = surfaceNormal.y * normalDotTranslationVector;
                        minimumTranslationVector.z = surfaceNormal.z * normalDotTranslationVector;
                    }
                }
            }
        }
        // Ignore micro-overlaps (contact offset)
        {
            float translationVectorLength =
                sqrtf(minimumTranslationVector.x * minimumTranslationVector.x +
                      minimumTranslationVector.y * minimumTranslationVector.y +
                      minimumTranslationVector.z * minimumTranslationVector.z);
            const float contactOffset = 0.06f; // small contact buffer to reduce jitter
            if (translationVectorLength < contactOffset)
                continue;
        }

        if (collisionAxis == 1 && minimumTranslationVector.y > 0 &&
            (playerCenter.y - collisionObjectCenter.y) >= 0.1f)
        {
            if (!hasGroundSeparationVector ||
                fabsf(minimumTranslationVector.y) < fabsf(groundSeparationVector.y))
            {
                groundSeparationVector = minimumTranslationVector;
                hasGroundSeparationVector = true;
            }
        }
        else if (collisionAxis == 1 && minimumTranslationVector.y < 0 &&
                 (playerCenter.y - collisionObjectCenter.y) <= -0.1f)
        {
            // Prevent pushing player down through ground
            if (!hasGroundSeparationVector ||
                fabsf(minimumTranslationVector.y) < fabsf(groundSeparationVector.y))
            {
                groundSeparationVector = minimumTranslationVector;
                hasGroundSeparationVector = true;
            }
        }
        else
        {
            // Additional jitter guard for small horizontal nudges
            if (fabsf(minimumTranslationVector.y) < 1e-4f)
            {
                float horizontalMagnitude =
                    sqrtf(minimumTranslationVector.x * minimumTranslationVector.x +
                          minimumTranslationVector.z * minimumTranslationVector.z);
                if (horizontalMagnitude < 0.15f) // ignore tiny side pushes while walking
                    continue;
            }
            float translationVectorLengthSquared =
                minimumTranslationVector.x * minimumTranslationVector.x +
                minimumTranslationVector.y * minimumTranslationVector.y +
                minimumTranslationVector.z * minimumTranslationVector.z;
            if (!hasOptimalResponse ||
                translationVectorLengthSquared < optimalSeparationDistanceSquared)
            {
                optimalSeparationDistanceSquared = translationVectorLengthSquared;
                optimalSeparationVector = minimumTranslationVector;
                hasOptimalResponse = true;
            }
        }
    }
    if (hasGroundSeparationVector)
    {
        collisionResponse = groundSeparationVector;

        // Cache the result and manage cache size
        const_cast<CollisionManager *>(this)->m_predictionCache[cacheHash] = {
            true, collisionResponse, const_cast<CollisionManager *>(this)->m_currentFrame};
        const_cast<CollisionManager *>(this)->ManageCacheSize();

        return true;
    }
    if (hasOptimalResponse)
    {
        collisionResponse = optimalSeparationVector;

        // Cache the result and manage cache size
        const_cast<CollisionManager *>(this)->m_predictionCache[cacheHash] = {
            true, collisionResponse, const_cast<CollisionManager *>(this)->m_currentFrame};
        const_cast<CollisionManager *>(this)->ManageCacheSize();

        return true;
    }

    // Cache negative result too and manage cache size
    const_cast<CollisionManager *>(this)->m_predictionCache[cacheHash] = {
        collisionDetected, collisionResponse, const_cast<CollisionManager *>(this)->m_currentFrame};
    const_cast<CollisionManager *>(this)->ManageCacheSize();

    return collisionDetected;
}

void CollisionManager::CreateAutoCollisionsFromModelsSelective(
    ModelLoader &models, const std::vector<std::string> &modelNames)
{
    constexpr size_t MAX_COLLISION_INSTANCES = 1000;
    int collisionObjectsCreated = 0;

    TraceLog(LOG_INFO,
             "Starting selective automatic collision generation for %zu specified models...",
             modelNames.size());

    // Prevent excessive collision creation that could cause memory issues
    if (modelNames.size() > MAX_COLLISION_INSTANCES)
    {
        TraceLog(LOG_ERROR,
                 "CollisionManager::CreateAutoCollisionsFromModelsSelective() - Too many models "
                 "(%zu), limiting to %zu",
                 modelNames.size(), MAX_COLLISION_INSTANCES);
        return;
    }

    // Create a set for faster lookup
    std::set<std::string> modelSet(modelNames.begin(), modelNames.end());

    // Get all available models
    auto availableModels = models.GetAvailableModels();
    TraceLog(LOG_INFO, "Found %zu models available, filtering to %zu specified models",
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
            TraceLog(LOG_DEBUG,
                     "Skipping collision creation for model '%s' (not in selective list)",
                     modelName.c_str());
            continue;
        }

        if (processedModelNames.find(modelName) != processedModelNames.end())
            continue;

        processedModelNames.insert(modelName);

        auto modelOpt = models.GetModelByName(modelName);
        if (!modelOpt)
        {
            TraceLog(LOG_WARNING, "CollisionManager - Model not found: %s", modelName.c_str());
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
        TraceLog(LOG_WARNING,
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

                    TraceLog(LOG_INFO, "Processing selective model: %s", task.modelName.c_str());

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
                        TraceLog(LOG_INFO, "Processing %zu/%zu instances for selective model '%s'",
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
                            TraceLog(LOG_WARNING,
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

    TraceLog(LOG_INFO,
             "Selective automatic collision generation complete. Created %d collision objects from "
             "%zu specified models",
             collisionObjectsCreated, modelNames.size());

    // Final spatial partitioning update for optimal performance
    UpdateSpatialPartitioning();

    TraceLog(LOG_INFO, "Spatial partitioning updated with %zu cells", m_spatialGrid.size());
}

// Helper function to create cache key
std::string CollisionManager::MakeCollisionCacheKey(const std::string &modelName, float scale) const
{
    // Round scale to 3 decimal places for better precision while avoiding cache misses
    int scaledInt = static_cast<int>(roundf(scale * 1000.0f));
    std::string key = modelName + "_s" + std::to_string(scaledInt);

    // Limit key length for performance
    if (key.length() > 64)
    {
        key = key.substr(0, 64);
    }

    return key;
}

bool CollisionManager::CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                                Vector3 position, float scale,
                                                const ModelLoader &models)
{
    TraceLog(LOG_INFO,
             "Creating collision from model '%s' at position (%.2f, %.2f, %.2f) scale=%.2f",
             modelName.c_str(), position.x, position.y, position.z, scale);

    // Validate inputs
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
    {
        TraceLog(LOG_ERROR, "Model '%s' has invalid position (%.2f, %.2f, %.2f)", modelName.c_str(),
                 position.x, position.y, position.z);
        return false;
    }

    if (!std::isfinite(scale) || scale <= 0.0f || scale > 1000.0f)
    {
        TraceLog(LOG_ERROR, "Model '%s' has invalid scale %.2f", modelName.c_str(), scale);
        return false;
    }

    // Validate model data before proceeding
    if (model.meshCount == 0)
    {
        TraceLog(LOG_ERROR, "Model '%s' has no meshes, cannot create collision", modelName.c_str());
        return false;
    }

    // Check for excessive mesh count that could cause memory issues
    if (model.meshCount > 1000)
    {
        TraceLog(LOG_ERROR, "Model '%s' has excessive mesh count (%d)", modelName.c_str(),
                 model.meshCount);
        return false;
    }

    // Check if model has any valid geometry
    bool hasValidGeometry = false;
    for (int i = 0; i < model.meshCount; i++)
    {
        const Mesh &mesh = model.meshes[i];
        if (mesh.vertices && mesh.indices && mesh.vertexCount > 0 && mesh.triangleCount > 0)
        {
            hasValidGeometry = true;
            break;
        }
    }

    if (!hasValidGeometry)
    {
        TraceLog(LOG_WARNING, "Model '%s' has no valid geometry, creating fallback AABB collision",
                 modelName.c_str());

        // Create fallback AABB collision using model bounds
        BoundingBox modelBounds = GetModelBoundingBox(model);
        Vector3 size = {modelBounds.max.x - modelBounds.min.x,
                        modelBounds.max.y - modelBounds.min.y,
                        modelBounds.max.z - modelBounds.min.z};
        Vector3 center = {(modelBounds.max.x + modelBounds.min.x) * 0.5f,
                          (modelBounds.max.y + modelBounds.min.y) * 0.5f,
                          (modelBounds.max.z + modelBounds.min.z) * 0.5f};

        Collision fallbackCollision(center, Vector3Scale(size, 0.5f * scale));
        fallbackCollision.SetCollisionType(CollisionType::AABB_ONLY);

        // Transform to instance position and scale
        fallbackCollision.Update(Vector3Add(center, position),
                                 Vector3Scale(Vector3Scale(size, 0.5f), scale));

        AddCollider(std::make_shared<Collision>(std::move(fallbackCollision)));
        return true;
    }

    // Get model configuration
    const ModelFileConfig *config = models.GetModelConfig(modelName);
    bool needsPreciseCollision =
        config && (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE ||
                   config->collisionPrecision == CollisionPrecision::BVH_ONLY ||
                   config->collisionPrecision == CollisionPrecision::IMPROVED_AABB ||
                   config->collisionPrecision == CollisionPrecision::AUTO);

    // If no explicit config or AUTO, analyze model shape to determine collision type
    if (!config || config->collisionPrecision == CollisionPrecision::AUTO)
    {
        needsPreciseCollision = AnalyzeModelShape(model, modelName);
    }

    // Get or create cached collision with improved caching strategy
    std::string cacheKey = MakeCollisionCacheKey(modelName, scale);
    std::shared_ptr<Collision> cachedCollision;

    auto cacheIt = m_collisionCache.find(cacheKey);
    if (cacheIt != m_collisionCache.end())
    {
        cachedCollision = cacheIt->second;
        TraceLog(LOG_DEBUG, "Using cached collision for '%s'", cacheKey.c_str());
    }
    else
    {
        // Create collision with optimized settings
        cachedCollision = CreateBaseCollision(model, modelName, config, needsPreciseCollision);

        // Only cache if it's actually useful (has geometry or is AABB)
        if (cachedCollision &&
            (cachedCollision->GetCollisionType() != CollisionType::AABB_ONLY ||
             cachedCollision->GetSize().x > 1.0f || cachedCollision->GetSize().z > 1.0f))
        {
            m_collisionCache[cacheKey] = cachedCollision;
            TraceLog(LOG_INFO, "Cached collision for '%s' (cache size: %zu)", cacheKey.c_str(),
                     m_collisionCache.size());
        }
    }

    // Create instance collision
    Collision instanceCollision;
    bool usePreciseForInstance =
        needsPreciseCollision &&
        (cachedCollision->GetCollisionType() == CollisionType::BVH_ONLY ||
         cachedCollision->GetCollisionType() == CollisionType::TRIANGLE_PRECISE);

    if (usePreciseForInstance &&
        m_preciseCollisionCountPerModel[modelName] < MAX_PRECISE_COLLISIONS_PER_MODEL)
    {
        // Use precise collision (prefer cached triangles)
        if (cachedCollision->HasTriangleData())
        {
            instanceCollision =
                CreatePreciseInstanceCollisionFromCached(*cachedCollision, position, scale);
        }
        else
        {
            instanceCollision = CreatePreciseInstanceCollision(model, position, scale, config);
        }
        m_preciseCollisionCountPerModel[modelName]++;
    }
    else
    {
        // Use simple AABB collision
        instanceCollision = CreateSimpleAABBInstanceCollision(*cachedCollision, position, scale);
        if (usePreciseForInstance)
        {
            TraceLog(LOG_WARNING,
                     "Reached limit of %d precise collision objects for model '%s', using AABB",
                     MAX_PRECISE_COLLISIONS_PER_MODEL, modelName.c_str());
        }
    }

    // Add to collision manager
    size_t beforeCount = GetColliders().size();

    try
    {
        AddCollider(std::make_shared<Collision>(std::move(instanceCollision)));

        bool success = GetColliders().size() > beforeCount;
        TraceLog(LOG_INFO, "%s created instance collision for '%s', collider count: %zu -> %zu",
                 success ? "Successfully" : "FAILED to", modelName.c_str(), beforeCount,
                 GetColliders().size());

        return success;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Failed to add collision for model '%s': %s", modelName.c_str(),
                 e.what());
        return false;
    }
    catch (...)
    {
        TraceLog(LOG_ERROR, "Unknown error occurred while adding collision for model '%s'",
                 modelName.c_str());
        return false;
    }
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
bool CollisionManager::CheckCollisionSpatial(const Collision &playerCollision) const
{
    const Vector3 playerMin = playerCollision.GetMin();
    const Vector3 playerMax = playerCollision.GetMax();

    // Calculate grid cells the player spans
    const float cellSize = 10.0f;
    int playerMinX = static_cast<int>(floorf(playerMin.x / cellSize));
    int playerMaxX = static_cast<int>(floorf(playerMax.x / cellSize));
    int playerMinZ = static_cast<int>(floorf(playerMin.z / cellSize));
    int playerMaxZ = static_cast<int>(floorf(playerMax.z / cellSize));

    // Check only objects in overlapping grid cells
    std::unordered_set<size_t> objectsToCheck;

    for (int x = playerMinX; x <= playerMaxX; ++x)
    {
        for (int z = playerMinZ; z <= playerMaxZ; ++z)
        {
            GridKey key = {x, z};
            auto it = m_spatialGrid.find(key);
            if (it != m_spatialGrid.end())
            {
                for (size_t objIndex : it->second)
                {
                    objectsToCheck.insert(objIndex);
                }
            }
        }
    }

    // Check collision against objects in relevant cells
    for (size_t objIndex : objectsToCheck)
    {
        if (objIndex >= m_collisionObjects.size())
            continue;

        const auto &collisionObject = m_collisionObjects[objIndex];
        bool collisionDetected = false;

        if (collisionObject->IsUsingBVH())
            collisionDetected = playerCollision.IntersectsBVH(*collisionObject);
        else
            collisionDetected = playerCollision.Intersects(*collisionObject);

        if (collisionDetected)
            return true;
    }

    return false;
}

// Helper method to analyze model shape and determine if it needs precise collision
bool CollisionManager::AnalyzeModelShape(const Model &model, const std::string &modelName)
{
    try
    {
        // Get model bounding box
        BoundingBox bounds = GetModelBoundingBox(model);
        Vector3 size = {bounds.max.x - bounds.min.x, bounds.max.y - bounds.min.y,
                        bounds.max.z - bounds.min.z};

        // Calculate aspect ratios to detect rectangular shapes
        float maxDim = fmaxf(fmaxf(size.x, size.y), size.z);
        float minDim = fminf(fminf(size.x, size.y), size.z);

        if (maxDim <= 0.0f || minDim <= 0.0f)
        {
            TraceLog(LOG_WARNING, "Model '%s' has invalid dimensions, defaulting to AABB",
                     modelName.c_str());
            return false; // Use AABB for invalid shapes
        }

        // Calculate rectangularity score (how close to a rectangular box)
        float aspectRatioXY = size.x / size.y;
        float aspectRatioXZ = size.x / size.z;
        float aspectRatioYZ = size.y / size.z;

        // For rectangular shapes, aspect ratios should be reasonable (not extreme)
        const float MAX_RECTANGULAR_RATIO = 10.0f; // Max ratio for rectangular detection
        bool isRectangular = (aspectRatioXY <= MAX_RECTANGULAR_RATIO &&
                              aspectRatioXY >= 1.0f / MAX_RECTANGULAR_RATIO) &&
                             (aspectRatioXZ <= MAX_RECTANGULAR_RATIO &&
                              aspectRatioXZ >= 1.0f / MAX_RECTANGULAR_RATIO) &&
                             (aspectRatioYZ <= MAX_RECTANGULAR_RATIO &&
                              aspectRatioYZ >= 1.0f / MAX_RECTANGULAR_RATIO);

        // Additional check: analyze triangle count and complexity
        int totalTriangles = 0;
        for (int i = 0; i < model.meshCount; i++)
        {
            const Mesh &mesh = model.meshes[i];
            if (mesh.triangleCount > 0)
            {
                totalTriangles += mesh.triangleCount;
            }
        }

        // If very few triangles and rectangular, definitely use AABB
        if (totalTriangles <= 12 && isRectangular)
        {
            TraceLog(LOG_DEBUG,
                     "Model '%s' detected as simple rectangular shape (%d triangles), using AABB",
                     modelName.c_str(), totalTriangles);
            return false; // Use AABB
        }

        // If many triangles or non-rectangular, use BVH
        if (totalTriangles > 100 || !isRectangular)
        {
            TraceLog(
                LOG_DEBUG,
                "Model '%s' detected as complex shape (%d triangles, rectangular=%s), using BVH",
                modelName.c_str(), totalTriangles, isRectangular ? "true" : "false");
            return true; // Use BVH
        }

        // Medium complexity: check if the shape is actually close to rectangular
        // by analyzing vertex distribution
        if (totalTriangles > 12 && totalTriangles <= 100)
        {
            // For medium complexity, do a more detailed analysis
            bool hasIrregularGeometry = AnalyzeGeometryIrregularity(model);
            if (!hasIrregularGeometry && isRectangular)
            {
                TraceLog(LOG_DEBUG, "Model '%s' medium complexity but regular geometry, using AABB",
                         modelName.c_str());
                return false; // Use AABB
            }
            else
            {
                TraceLog(LOG_DEBUG,
                         "Model '%s' medium complexity with irregular geometry, using BVH",
                         modelName.c_str());
                return true; // Use BVH
            }
        }

        // Default fallback
        TraceLog(LOG_DEBUG, "Model '%s' analysis inconclusive, defaulting to AABB",
                 modelName.c_str());
        return false; // Use AABB as safe default
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_WARNING, "Shape analysis failed for model '%s': %s, defaulting to AABB",
                 modelName.c_str(), e.what());
        return false; // Use AABB on analysis failure
    }
}

// Helper method to analyze if geometry has irregular features that would benefit from BVH
bool CollisionManager::AnalyzeGeometryIrregularity(const Model &model)
{
    try
    {
        // Simple irregularity check: look for non-planar faces or complex vertex arrangements
        // This is a basic implementation - could be enhanced with more sophisticated analysis

        int irregularFeatures = 0;
        int totalFaces = 0;

        for (int meshIdx = 0; meshIdx < model.meshCount; meshIdx++)
        {
            const Mesh &mesh = model.meshes[meshIdx];
            if (!mesh.vertices || !mesh.indices || mesh.vertexCount == 0 || mesh.triangleCount == 0)
                continue;

            totalFaces += mesh.triangleCount;

            // Check for irregular vertex patterns (simplified analysis)
            // In a real implementation, you might check for curvature, holes, etc.
            if (mesh.vertexCount > mesh.triangleCount * 2)
            {
                irregularFeatures++; // Many vertices relative to faces suggests complexity
            }
        }

        // If more than 30% of meshes show irregular features, consider it irregular
        return (irregularFeatures * 3) > totalFaces;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_WARNING, "Geometry irregularity analysis failed: %s", e.what());
        return true; // Assume irregular on failure to be safe
    }
}

// Helper method to create base collision for caching
std::shared_ptr<Collision> CollisionManager::CreateBaseCollision(const Model &model,
                                                                 const std::string &modelName,
                                                                 const ModelFileConfig *config,
                                                                 bool needsPreciseCollision)
{
    std::shared_ptr<Collision> collision;

    try
    {
        // Validate model data before proceeding
        if (model.meshCount == 0)
        {
            TraceLog(LOG_ERROR, "Model '%s' has no meshes, creating fallback collision",
                     modelName.c_str());
            BoundingBox modelBounds = GetModelBoundingBox(model);
            Vector3 size = {modelBounds.max.x - modelBounds.min.x,
                            modelBounds.max.y - modelBounds.min.y,
                            modelBounds.max.z - modelBounds.min.z};
            Vector3 center = {(modelBounds.max.x + modelBounds.min.x) * 0.5f,
                              (modelBounds.max.y + modelBounds.min.y) * 0.5f,
                              (modelBounds.max.z + modelBounds.min.z) * 0.5f};

            collision = std::make_shared<Collision>(center, Vector3Scale(size, 0.5f));
            collision->SetCollisionType(CollisionType::AABB_ONLY);
            return collision;
        }

        // Check if model has any valid geometry
        bool hasValidGeometry = false;
        for (int i = 0; i < model.meshCount; i++)
        {
            const Mesh &mesh = model.meshes[i];
            if (mesh.vertices && mesh.indices && mesh.vertexCount > 0 && mesh.triangleCount > 0)
            {
                hasValidGeometry = true;
                break;
            }
        }

        if (!hasValidGeometry)
        {
            // Fallback AABB for models without geometry
            TraceLog(LOG_WARNING, "Model '%s' has no valid geometry, creating fallback collision",
                     modelName.c_str());
            BoundingBox modelBounds = GetModelBoundingBox(model);
            Vector3 size = {modelBounds.max.x - modelBounds.min.x,
                            modelBounds.max.y - modelBounds.min.y,
                            modelBounds.max.z - modelBounds.min.z};
            Vector3 center = {(modelBounds.max.x + modelBounds.min.x) * 0.5f,
                              (modelBounds.max.y + modelBounds.min.y) * 0.5f,
                              (modelBounds.max.z + modelBounds.min.z) * 0.5f};

            collision = std::make_shared<Collision>(center, Vector3Scale(size, 0.5f));
            collision->SetCollisionType(CollisionType::AABB_ONLY);
        }
        else
        {
            // Create collision from model geometry with error handling
            collision = std::make_shared<Collision>();

            try
            {
                Model modelCopy = model; // Create a copy for collision building
                collision->BuildFromModel(&modelCopy, MatrixIdentity());

                // Set collision type based on configuration
                if (needsPreciseCollision && config)
                {
                    CollisionType targetType = CollisionType::HYBRID_AUTO;

                    switch (config->collisionPrecision)
                    {
                    case CollisionPrecision::TRIANGLE_PRECISE:
                        targetType = CollisionType::TRIANGLE_PRECISE;
                        break;
                    case CollisionPrecision::BVH_ONLY:
                        targetType = CollisionType::BVH_ONLY;
                        break;
                    case CollisionPrecision::AUTO:
                    default:
                        targetType = CollisionType::HYBRID_AUTO;
                        break;
                    }

                    collision->SetCollisionType(targetType);
                }
                else
                {
                    collision->SetCollisionType(CollisionType::AABB_ONLY);
                }
            }
            catch (const std::exception &e)
            {
                TraceLog(LOG_ERROR, "Failed to build collision from model '%s': %s",
                         modelName.c_str(), e.what());

                // Fallback to AABB collision
                BoundingBox modelBounds = GetModelBoundingBox(model);
                Vector3 size = {modelBounds.max.x - modelBounds.min.x,
                                modelBounds.max.y - modelBounds.min.y,
                                modelBounds.max.z - modelBounds.min.z};
                Vector3 center = {(modelBounds.max.x + modelBounds.min.x) * 0.5f,
                                  (modelBounds.max.y + modelBounds.min.y) * 0.5f,
                                  (modelBounds.max.z + modelBounds.min.z) * 0.5f};

                *collision = Collision(center, Vector3Scale(size, 0.5f));
                collision->SetCollisionType(CollisionType::AABB_ONLY);
            }
        }
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Critical error creating collision for model '%s': %s",
                 modelName.c_str(), e.what());

        // Create emergency fallback collision
        collision = std::make_shared<Collision>(Vector3{0, 0, 0}, Vector3{1, 1, 1});
        collision->SetCollisionType(CollisionType::AABB_ONLY);
    }

    return collision;
}

Collision CollisionManager::CreatePreciseInstanceCollision(const Model &model, Vector3 position,
                                                           float scale,
                                                           const ModelFileConfig *config)
{
    Collision instanceCollision;

    Matrix transform = MatrixIdentity();
    transform = MatrixMultiply(transform, MatrixScale(scale, scale, scale));
    // If there is rotation:
    // transform = MatrixMultiply(transform, MatrixRotateXYZ(rotation));
    transform = MatrixMultiply(transform, MatrixTranslate(position.x, position.y, position.z));

    Model modelCopy = model;

    if (config)
        instanceCollision.BuildFromModel(&modelCopy, transform);

    instanceCollision.SetCollisionType(CollisionType::BVH_ONLY);

    TraceLog(LOG_INFO, "Built BVH collision for instance at (%.2f, %.2f, %.2f)", position.x,
             position.y, position.z);

    return instanceCollision;
}

Collision
CollisionManager::CreatePreciseInstanceCollisionFromCached(const Collision &cachedCollision,
                                                           Vector3 position, float scale)
{

    Collision instance;

    Matrix transform = MatrixIdentity();
    transform = MatrixMultiply(transform, MatrixScale(scale, scale, scale));
    transform = MatrixMultiply(transform, MatrixTranslate(position.x, position.y, position.z));

    const auto &tris = cachedCollision.GetTriangles();
    instance = Collision{};
    for (const auto &t : tris)
    {
        Vector3 v0 = Vector3Transform(t.V0(), transform);
        Vector3 v1 = Vector3Transform(t.V1(), transform);
        Vector3 v2 = Vector3Transform(t.V2(), transform);
        instance.AddTriangle(CollisionTriangle(v0, v1, v2));
    }

    instance.UpdateAABBFromTriangles();
    instance.InitializeBVH();
    instance.SetCollisionType(CollisionType::BVH_ONLY);
    return instance;
}

Collision CollisionManager::CreateSimpleAABBInstanceCollision(const Collision &cachedCollision,
                                                              const Vector3 &position, float scale)
{
    Vector3 transformedCenter =
        Vector3Add(Vector3Scale(cachedCollision.GetCenter(), scale), position);
    Vector3 scaledSize = Vector3Scale(cachedCollision.GetSize(), scale);

    // Collision expects half-size extents
    Collision instanceCollision(transformedCenter, Vector3Scale(scaledSize, 0.5f));
    instanceCollision.SetCollisionType(CollisionType::AABB_ONLY);
    return instanceCollision;
}

// Prediction cache management
void CollisionManager::UpdateFrameCache()
{
    m_currentFrame++;
    if (m_currentFrame % 60 == 0) // Clean cache every 60 frames
    {
        ClearExpiredCache();
    }
}

void CollisionManager::ClearExpiredCache()
{
    auto it = m_predictionCache.begin();
    while (it != m_predictionCache.end())
    {
        if (m_currentFrame - it->second.frameCount > CACHE_LIFETIME_FRAMES)
        {
            it = m_predictionCache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

size_t CollisionManager::GetPredictionCacheHash(const Collision &playerCollision) const
{
    // Simple hash based on player collision bounds and position
    Vector3 min = playerCollision.GetMin();
    Vector3 max = playerCollision.GetMax();

    // Use a simple combination of position and size for hashing
    size_t hash = 0;
    hash = std::hash<float>{}(min.x) ^ std::hash<float>{}(min.y) ^ std::hash<float>{}(min.z);
    hash = hash * 31 + std::hash<float>{}(max.x) ^ std::hash<float>{}(max.y) ^
           std::hash<float>{}(max.z);

    return hash;
}

void CollisionManager::ManageCacheSize()
{
    if (m_predictionCache.size() > MAX_PREDICTION_CACHE_SIZE)
    {
        // Remove oldest entries (simple LRU-like behavior)
        size_t entriesToRemove = m_predictionCache.size() - MAX_PREDICTION_CACHE_SIZE;
        auto it = m_predictionCache.begin();

        {
            it = m_predictionCache.erase(it);
        }
    }
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
    UpdateEntitySpatialPartitioning(); // Rebuild grid immediately or defer
}

void CollisionManager::RemoveEntityCollider(ECS::EntityID entity)
{
    m_entityColliders.erase(entity);
    // Note: optimization - we might want to just remove it from grid instead of full rebuild
}

void CollisionManager::UpdateEntityCollider(ECS::EntityID entity, const Vector3 &position)
{
    auto it = m_entityColliders.find(entity);
    if (it != m_entityColliders.end() && it->second)
    {
        Collision &col = *it->second;
        Vector3 currentSize = col.GetSize();
        // Force update position
        col.Update(position, currentSize);
    }
}

std::shared_ptr<Collision> CollisionManager::GetEntityCollider(ECS::EntityID entity) const
{
    auto it = m_entityColliders.find(entity);
    if (it != m_entityColliders.end())
        return it->second;
    return nullptr;
}

void CollisionManager::UpdateEntitySpatialPartitioning()
{
    m_entitySpatialGrid.clear();
    // Use smaller grid size for entities if needed, or same
    float cellSize = m_entityGridCellSize;

    for (const auto &[entity, collider] : m_entityColliders)
    {
        Vector3 min = collider->GetMin();
        Vector3 max = collider->GetMax();

        int minX = static_cast<int>(floorf(min.x / cellSize));
        int maxX = static_cast<int>(floorf(max.x / cellSize));
        int minZ = static_cast<int>(floorf(min.z / cellSize));
        int maxZ = static_cast<int>(floorf(max.z / cellSize));

        for (int x = minX; x <= maxX; ++x)
        {
            for (int z = minZ; z <= maxZ; ++z)
            {
                GridKey key = {x, z};
                m_entitySpatialGrid[key].push_back(entity);
            }
        }
    }
}

bool CollisionManager::CheckEntityCollision(ECS::EntityID selfEntity, const Collision &collider,
                                            std::vector<ECS::EntityID> &outCollidedEntities) const
{
    bool collisionDetected = false;
    Vector3 min = collider.GetMin();
    Vector3 max = collider.GetMax();

    int minX = static_cast<int>(floorf(min.x / m_entityGridCellSize));
    int maxX = static_cast<int>(floorf(max.x / m_entityGridCellSize));
    int minZ = static_cast<int>(floorf(min.z / m_entityGridCellSize));
    int maxZ = static_cast<int>(floorf(max.z / m_entityGridCellSize));

    // Keep track of checked entities to avoid duplicates
    static thread_local std::vector<ECS::EntityID> checkedEntities; // Simple optimization
    checkedEntities.clear();

    for (int x = minX; x <= maxX; ++x)
    {
        for (int z = minZ; z <= maxZ; ++z)
        {
            GridKey key = {x, z};
            auto it = m_entitySpatialGrid.find(key);
            if (it == m_entitySpatialGrid.end())
                continue;

            for (ECS::EntityID otherEntity : it->second)
            {
                if (otherEntity == selfEntity)
                    continue;

                // Check if already processed
                bool alreadyChecked = false;
                for (auto checked : checkedEntities)
                {
                    if (checked == otherEntity)
                    {
                        alreadyChecked = true;
                        break;
                    }
                }
                if (alreadyChecked)
                    continue;

                checkedEntities.push_back(otherEntity);

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
        }
    }

    return collisionDetected;
}




