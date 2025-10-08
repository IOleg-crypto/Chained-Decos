#include <CollisionManager.h>
#include <CollisionSystem.h>
#include <Model/Model.h>
#include <algorithm>
#include <compare>
#include <raylib.h>
#include <set>
#include <string>
#include <vector>
#include <cfloat>
#include <execution>
#include <future>
#include <thread>

void CollisionManager::Initialize() const
{
    for (auto &collisionObject : m_collisionObjects)
    {
        if (collisionObject->GetCollisionType() == CollisionType::BVH_ONLY ||
            collisionObject->GetCollisionType() == CollisionType::TRIANGLE_PRECISE)
        {
            collisionObject->InitializeBVH();
        }
    }

    TraceLog(LOG_INFO, "CollisionManager initialized with %zu collision objects", m_collisionObjects.size());
}

void CollisionManager::AddCollider(Collision &&collisionObject)
{
    m_collisionObjects.push_back(std::make_unique<Collision>(std::move(collisionObject)));

    if (m_collisionObjects.back()->GetCollisionType() == CollisionType::BVH_ONLY ||
        m_collisionObjects.back()->GetCollisionType() == CollisionType::TRIANGLE_PRECISE)
    {
        m_collisionObjects.back()->InitializeBVH();
    }

    TraceLog(LOG_INFO, "Added collision object, total count: %zu", m_collisionObjects.size());
}

void CollisionManager::AddColliderRef(Collision* collisionObject)
{
    if (!collisionObject) return;

    m_collisionObjects.push_back(std::unique_ptr<Collision>(collisionObject));

    if (collisionObject->GetCollisionType() == CollisionType::BVH_ONLY ||
        collisionObject->GetCollisionType() == CollisionType::TRIANGLE_PRECISE)
    {
        collisionObject->InitializeBVH();
    }

    TraceLog(LOG_INFO, "Added collision object reference, total count: %zu", m_collisionObjects.size());
}

void CollisionManager::ClearColliders() { m_collisionObjects.clear(); }

bool CollisionManager::CheckCollision(const Collision &playerCollision) const
{
    if (m_collisionObjects.empty())
        return false;

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
bool CollisionManager::CheckCollision(const Collision &playerCollision, Vector3 &collisionResponse) const
{
    if (m_collisionObjects.empty())
        return false;
    const Vector3 playerMin = playerCollision.GetMin();
    const Vector3 playerMax = playerCollision.GetMax();
    const Vector3 playerCenter = {(playerMin.x + playerMax.x) * 0.5f,
                                   (playerMin.y + playerMax.y) * 0.5f,
                                   (playerMin.z + playerMax.z) * 0.5f};
    bool collisionDetected = false;
    collisionResponse = (Vector3){0, 0, 0};
    bool hasOptimalResponse = false;
    Vector3 optimalSeparationVector = {0, 0, 0};
    float optimalSeparationDistanceSquared = FLT_MAX;
    Vector3 groundSeparationVector = {0, 0, 0};
    bool hasGroundSeparationVector = false;
    for (const auto &collisionObject : m_collisionObjects)
    {
        bool collisionDetectedInObject = collisionObject->IsUsingBVH() ? playerCollision.IntersectsBVH(*collisionObject)
                                           : playerCollision.Intersects(*collisionObject);
        if (!collisionDetectedInObject)
            continue;
        collisionDetected = true;

        if (collisionObject->IsUsingBVH()) {
            Vector3 raycastOrigin = playerCenter;
            Vector3 raycastDirection = {0.0f, -1.0f, 0.0f};
            float maxRaycastDistance = (playerMax.y - playerMin.y) + 1.0f;
            RayHit raycastHit;
            if (collisionObject->RaycastBVH(raycastOrigin, raycastDirection, maxRaycastDistance, raycastHit) && raycastHit.hit) {
                float upwardDelta = raycastHit.position.y - playerMin.y;
                if (upwardDelta > 0.0f && upwardDelta < maxRaycastDistance) {
                    Vector3 refinedSeparationVector = {0.0f, upwardDelta, 0.0f};
                    if (!hasGroundSeparationVector || fabsf(refinedSeparationVector.y) < fabsf(groundSeparationVector.y)) {
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
        Vector3 minimumTranslationVector = {0, 0, 0};
        switch (collisionAxis)
        {
        case 0:
            minimumTranslationVector.x = (playerCenter.x < collisionObjectCenter.x) ? -minimumOverlap : minimumOverlap;
            break;
        case 1:
            minimumTranslationVector.y = (playerCenter.y < collisionObjectCenter.y) ? -minimumOverlap : minimumOverlap;
            break;
        case 2:
            minimumTranslationVector.z = (playerCenter.z < collisionObjectCenter.z) ? -minimumOverlap : minimumOverlap;
            break;
        default:
            break;
        }

        // For BVH colliders: align MTV with triangle normal to better handle slopes/uneven surfaces
        if (collisionObject->IsUsingBVH())
        {
            Vector3 surfaceNormalDirection = minimumTranslationVector;
            float surfaceNormalDirectionLength = sqrtf(surfaceNormalDirection.x * surfaceNormalDirection.x + surfaceNormalDirection.y * surfaceNormalDirection.y + surfaceNormalDirection.z * surfaceNormalDirection.z);
            if (surfaceNormalDirectionLength > 1e-5f)
            {
                surfaceNormalDirection.x /= surfaceNormalDirectionLength; surfaceNormalDirection.y /= surfaceNormalDirectionLength; surfaceNormalDirection.z /= surfaceNormalDirectionLength;
                // Cast from player center opposite to MTV to find nearest contact and normal
                RayHit normalHit; normalHit.hit = false;
                if (collisionObject->RaycastBVH(playerCenter, (Vector3){-surfaceNormalDirection.x, -surfaceNormalDirection.y, -surfaceNormalDirection.z}, fminf(surfaceNormalDirectionLength + 0.5f, 2.0f), normalHit) && normalHit.hit)
                {
                    Vector3 surfaceNormal = normalHit.normal;
                    float normalDotTranslationVector = surfaceNormal.x * minimumTranslationVector.x + surfaceNormal.y * minimumTranslationVector.y + surfaceNormal.z * minimumTranslationVector.z;
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
            float translationVectorLength = sqrtf(minimumTranslationVector.x*minimumTranslationVector.x + minimumTranslationVector.y*minimumTranslationVector.y + minimumTranslationVector.z*minimumTranslationVector.z);
            const float contactOffset = 0.06f; // small contact buffer to reduce jitter
            if (translationVectorLength < contactOffset)
                continue;
        }

        if (collisionAxis == 1 && minimumTranslationVector.y > 0 && (playerCenter.y - collisionObjectCenter.y) >= 0.1f)
        {
            if (!hasGroundSeparationVector || fabsf(minimumTranslationVector.y) < fabsf(groundSeparationVector.y))
            {
                groundSeparationVector = minimumTranslationVector;
                hasGroundSeparationVector = true;
            }
        }
        else if (collisionAxis == 1 && minimumTranslationVector.y < 0 && (playerCenter.y - collisionObjectCenter.y) <= -0.1f)
        {
            // Prevent pushing player down through ground
            if (!hasGroundSeparationVector || fabsf(minimumTranslationVector.y) < fabsf(groundSeparationVector.y))
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
                float horizontalMagnitude = sqrtf(minimumTranslationVector.x*minimumTranslationVector.x + minimumTranslationVector.z*minimumTranslationVector.z);
                if (horizontalMagnitude < 0.15f) // ignore tiny side pushes while walking
                    continue;
            }
            float translationVectorLengthSquared = minimumTranslationVector.x * minimumTranslationVector.x + minimumTranslationVector.y * minimumTranslationVector.y + minimumTranslationVector.z * minimumTranslationVector.z;
            if (!hasOptimalResponse || translationVectorLengthSquared < optimalSeparationDistanceSquared)
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
        return true;
    }
    if (hasOptimalResponse)
    {
        collisionResponse = optimalSeparationVector;
        return true;
    }
    return collisionDetected;
}

void CollisionManager::CreateAutoCollisionsFromModels(ModelLoader &models)
{
    TraceLog(LOG_INFO, "Starting automatic collision generation for all models...");

    // Get all available models
    auto availableModels = models.GetAvailableModels();
    TraceLog(LOG_INFO, "Found %zu models to check", availableModels.size());

    // Track processed models to avoid duplication
    std::set<std::string> processedModelNames;
    int collisionObjectsCreated = 0;
    constexpr size_t MAX_COLLISION_INSTANCES = 3; // Reduced safety limit for better performance

    // Process each model
    for (const auto &modelName : availableModels)
    {
        // Skip if already processed
        if (processedModelNames.contains(modelName))
        {
            continue;
        }

        processedModelNames.insert(modelName);
        TraceLog(LOG_INFO, "Processing model: %s", modelName.c_str());

        try
        {
            // Get the model and check if it should have collision
            Model &model = models.GetModelByName(modelName);
            bool hasCollision = models.HasCollision(modelName);

            // Skip models without collision or meshes
            if (!hasCollision || model.meshCount == 0)
            {
                TraceLog(LOG_INFO, "Skipping model '%s': hasCollision=%s, meshCount=%d",
                         modelName.c_str(), hasCollision ? "true" : "false", model.meshCount);
                continue;
            }

            // Find instances of this model
            auto instances = models.GetInstancesByTag(modelName);

            if (instances.empty())
            {
                // No instances found, create default collision
                Vector3 defaultPos = (modelName == "arc") ? Vector3{0, 0, 140} : Vector3{0, 0, 0};
                if (CreateCollisionFromModel(model, modelName, defaultPos, 1.0f, models))
                {
                    collisionObjectsCreated++;
                }
            }
            else
            {
                // Create collisions for each instance (up to the limit)
                size_t instanceLimit = std::min(instances.size(), MAX_COLLISION_INSTANCES);
                TraceLog(LOG_INFO, "Processing %zu/%zu instances for model '%s'", instanceLimit,
                         instances.size(), modelName.c_str());

                for (size_t i = 0; i < instanceLimit; i++)
                {
                    auto *instance = instances[i];
                    Vector3 position = instance->GetModelPosition();
                    float scale = instance->GetScale();

                    if (CreateCollisionFromModel(model, modelName, position, scale, models))
                    {
                        collisionObjectsCreated++;
                    }
                }

                if (instances.size() > MAX_COLLISION_INSTANCES)
                {
                    TraceLog(LOG_WARNING,
                             "Limited collisions for model '%s' to %zu (of %zu instances)",
                             modelName.c_str(), MAX_COLLISION_INSTANCES, instances.size());
                }
            }
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Failed to create collision for model '%s': %s", modelName.c_str(),
                     e.what());
        }
    }

    TraceLog(LOG_INFO,
              "Automatic collision generation complete. Created %d collision objects from %zu models",
              collisionObjectsCreated, availableModels.size());
}

// Helper function to create cache key
std::string CollisionManager::MakeCollisionCacheKey(const std::string &modelName, float scale) const
{
    // Round scale to 1 decimal place to avoid cache misses for tiny differences
    int scaledInt = static_cast<int>(scale * 10.0f);
    std::string key = modelName + "_s" + std::to_string(scaledInt);
    TraceLog(LOG_INFO, "Generated cache key: %s", key.c_str());
    return key;
}

bool CollisionManager::CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                                Vector3 position, float scale,
                                                const ModelLoader &models)
{
    TraceLog(LOG_INFO,
             "Creating collision from model '%s' at position (%.2f, %.2f, %.2f) scale=%.2f",
             modelName.c_str(), position.x, position.y, position.z, scale);

    // Get model configuration
    const ModelFileConfig *config = models.GetModelConfig(modelName);
    bool needsPreciseCollision = config && (
        config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE ||
        config->collisionPrecision == CollisionPrecision::BVH_ONLY ||
        config->collisionPrecision == CollisionPrecision::IMPROVED_AABB ||
        config->collisionPrecision == CollisionPrecision::AUTO);

    // Get or create cached collision
    std::string cacheKey = MakeCollisionCacheKey(modelName, scale);
    std::shared_ptr<Collision> cachedCollision;
    
    auto cacheIt = m_collisionCache.find(cacheKey);
    if (cacheIt != m_collisionCache.end())
    {
        cachedCollision = cacheIt->second;
        TraceLog(LOG_INFO, "Using cached collision for '%s'", cacheKey.c_str());
    }
    else
    {
        cachedCollision = CreateBaseCollision(model, modelName, config, needsPreciseCollision);
        m_collisionCache[cacheKey] = cachedCollision;
    }

    // Create instance collision
    Collision instanceCollision;
    bool usePreciseForInstance = needsPreciseCollision && 
        (cachedCollision->GetCollisionType() == CollisionType::BVH_ONLY || 
         cachedCollision->GetCollisionType() == CollisionType::TRIANGLE_PRECISE);

    if (usePreciseForInstance && m_preciseCollisionCountPerModel[modelName] < MAX_PRECISE_COLLISIONS_PER_MODEL)
    {
        // Use precise collision (prefer cached triangles)
        if (cachedCollision->HasTriangleData())
        {
            instanceCollision = CreatePreciseInstanceCollisionFromCached(*cachedCollision, position, scale);
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
            TraceLog(LOG_WARNING, "Reached limit of %d precise collision objects for model '%s', using AABB",
                      MAX_PRECISE_COLLISIONS_PER_MODEL, modelName.c_str());
        }
    }

    // Add to collision manager
    size_t beforeCount = GetColliders().size();
    AddCollider(std::move(instanceCollision));
    
    bool success = GetColliders().size() > beforeCount;
    TraceLog(LOG_INFO, "%s created instance collision for '%s', collider count: %zu -> %zu",
             success ? "Successfully" : "FAILED to", modelName.c_str(), beforeCount, GetColliders().size());
    
    return success;
}

const std::vector<std::unique_ptr<Collision>> &CollisionManager::GetColliders() const
{
    return m_collisionObjects;
}

bool CollisionManager::RaycastDown(const Vector3 &raycastOrigin, float maxRaycastDistance, float &hitDistance,
                                    Vector3 &hitPoint, Vector3 &hitNormal) const
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
            if (collisionObject->RaycastBVH(raycastOrigin, raycastDirection, maxRaycastDistance, raycastHit))
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
            // Ray is vertical down. Intersect at y = mx.y (top face)
            if (raycastOrigin.y >= mx.y)
            {
                float dist = raycastOrigin.y - mx.y;
                if (dist <= maxRaycastDistance)
                {
                    // Check if x,z are inside bounds at that y
                    if (raycastOrigin.x >= mn.x && raycastOrigin.x <= mx.x && raycastOrigin.z >= mn.z && raycastOrigin.z <= mx.z)
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
            }
            // Also check bottom face for ground collision
            else if (raycastOrigin.y <= mn.y)
            {
                float dist = mn.y - raycastOrigin.y;
                if (dist <= maxRaycastDistance)
                {
                    // Check if x,z are inside bounds at that y
                    if (raycastOrigin.x >= mn.x && raycastOrigin.x <= mx.x && raycastOrigin.z >= mn.z && raycastOrigin.z <= mx.z)
                    {
                        if (dist < nearestHitDistance)
                        {
                            nearestHitDistance = dist;
                            nearestHitPoint = {raycastOrigin.x, mn.y, raycastOrigin.z};
                            nearestHitNormal = {0, -1, 0};
                            anyHitDetected = true;
                        }
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

// Parallel version of collision checking for better performance with many objects
bool CollisionManager::CheckCollisionParallel(const Collision &playerCollision) const
{
    if (m_collisionObjects.empty())
        return false;

    // Use parallel execution policy for better performance with many collision objects
    size_t numObjects = m_collisionObjects.size();
    const size_t parallelThreshold = 16; // Only use parallel processing if we have enough objects

    if (numObjects < parallelThreshold) {
        // Fall back to sequential for small numbers of objects
        return CheckCollision(playerCollision);
    }

    // Split collision objects into chunks for parallel processing
    size_t numThreads = std::min(static_cast<size_t>(std::thread::hardware_concurrency()), numObjects / 4);
    numThreads = std::max(numThreads, static_cast<size_t>(1));

    std::vector<std::future<bool>> futures;
    size_t chunkSize = numObjects / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        size_t startIdx = i * chunkSize;
        size_t endIdx = (i == numThreads - 1) ? numObjects : (i + 1) * chunkSize;

        futures.push_back(std::async(std::launch::async, [this, &playerCollision, startIdx, endIdx]() {
            for (size_t j = startIdx; j < endIdx; ++j) {
                const auto &collisionObject = m_collisionObjects[j];
                bool collisionDetectedInObject = false;

                if (collisionObject->IsUsingBVH())
                    collisionDetectedInObject = playerCollision.IntersectsBVH(*collisionObject);
                else
                    collisionDetectedInObject = playerCollision.Intersects(*collisionObject);

                if (collisionDetectedInObject)
                    return true;
            }
            return false;
        }));
    }

    // Check if any thread found a collision
    for (auto &future : futures) {
        if (future.get())
            return true;
    }

    return false;
}

// Parallel version with response vector
bool CollisionManager::CheckCollisionParallel(const Collision &playerCollision, Vector3 &collisionResponse) const
{
    if (m_collisionObjects.empty())
        return false;

    const size_t parallelThreshold = 16;

    if (m_collisionObjects.size() < parallelThreshold) {
        return CheckCollision(playerCollision, collisionResponse);
    }

    // For parallel processing with response, we need to collect all collision data
    // and then resolve conflicts sequentially to ensure correct physics behavior
    std::vector<std::future<std::pair<bool, Vector3>>> futures;

    size_t numThreads = std::min(static_cast<size_t>(std::thread::hardware_concurrency()), m_collisionObjects.size() / 4);
    size_t chunkSize = m_collisionObjects.size() / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        size_t startIdx = i * chunkSize;
        size_t endIdx = (i == numThreads - 1) ? m_collisionObjects.size() : (i + 1) * chunkSize;

        futures.push_back(std::async(std::launch::async, [this, &playerCollision, startIdx, endIdx]() {
            std::pair<bool, Vector3> result = {false, {0, 0, 0}};
            Vector3 localResponse = {0, 0, 0};

            for (size_t j = startIdx; j < endIdx; ++j) {
                const auto &collisionObject = m_collisionObjects[j];
                if (CheckCollisionSingleObject(playerCollision, *collisionObject, localResponse)) {
                    result.first = true;
                    // For now, just use the first collision found
                    // In a more sophisticated implementation, we could collect all collisions
                    result.second = localResponse;
                    break;
                }
            }

            return result;
        }));
    }

    // Collect results and find the best collision response
    bool anyCollision = false;
    Vector3 bestResponse = {0, 0, 0};
    float bestDistanceSquared = FLT_MAX;

    for (auto &future : futures) {
        auto [hasCollision, response] = future.get();
        if (hasCollision) {
            anyCollision = true;
            float distanceSquared = response.x * response.x + response.y * response.y + response.z * response.z;
            if (distanceSquared < bestDistanceSquared) {
                bestDistanceSquared = distanceSquared;
                bestResponse = response;
            }
        }
    }

    if (anyCollision) {
        collisionResponse = bestResponse;
        return true;
    }

    return false;
}

// Helper method to check collision with a single object (extracted for parallel use)
bool CollisionManager::CheckCollisionSingleObject(const Collision &playerCollision,
                                                  const Collision &collisionObject,
                                                  Vector3 &response) const
{
    const Vector3 playerMin = playerCollision.GetMin();
    const Vector3 playerMax = playerCollision.GetMax();
    const Vector3 playerCenter = {(playerMin.x + playerMax.x) * 0.5f,
                                   (playerMin.y + playerMax.y) * 0.5f,
                                   (playerMin.z + playerMax.z) * 0.5f};

    bool collisionDetectedInObject = collisionObject.IsUsingBVH() ?
        playerCollision.IntersectsBVH(collisionObject) :
        playerCollision.Intersects(collisionObject);

    if (!collisionDetectedInObject)
        return false;

    // Calculate MTV for this specific collision
    const Vector3 collisionObjectMin = collisionObject.GetMin();
    const Vector3 collisionObjectMax = collisionObject.GetMax();
    const Vector3 collisionObjectCenter = {(collisionObjectMin.x + collisionObjectMax.x) * 0.5f,
                                         (collisionObjectMin.y + collisionObjectMax.y) * 0.5f,
                                         (collisionObjectMin.z + collisionObjectMax.z) * 0.5f};

    const float overlapX = fminf(playerMax.x, collisionObjectMax.x) - fmaxf(playerMin.x, collisionObjectMin.x);
    const float overlapY = fminf(playerMax.y, collisionObjectMax.y) - fmaxf(playerMin.y, collisionObjectMin.y);
    const float overlapZ = fminf(playerMax.z, collisionObjectMax.z) - fmaxf(playerMin.z, collisionObjectMin.z);

    if (overlapX <= 0 || overlapY <= 0 || overlapZ <= 0)
        return false;

    float minimumOverlap = fabsf(overlapX);
    int collisionAxis = 0;

    if (fabsf(overlapY) < minimumOverlap) {
        minimumOverlap = fabsf(overlapY);
        collisionAxis = 1;
    }
    if (fabsf(overlapZ) < minimumOverlap) {
        minimumOverlap = fabsf(overlapZ);
        collisionAxis = 2;
    }

    Vector3 minimumTranslationVector = {0, 0, 0};
    switch (collisionAxis) {
        case 0:
            minimumTranslationVector.x = (playerCenter.x < collisionObjectCenter.x) ? -minimumOverlap : minimumOverlap;
            break;
        case 1:
            minimumTranslationVector.y = (playerCenter.y < collisionObjectCenter.y) ? -minimumOverlap : minimumOverlap;
            break;
        case 2:
            minimumTranslationVector.z = (playerCenter.z < collisionObjectCenter.z) ? -minimumOverlap : minimumOverlap;
            break;
        default:
            break;
    }

    response = minimumTranslationVector;
    return true;
}

// Helper method to create base collision for caching
std::shared_ptr<Collision> CollisionManager::CreateBaseCollision(const Model &model,
                                                                 const std::string &modelName,
                                                                 const ModelFileConfig *config,
                                                                 bool needsPreciseCollision)
{
    std::shared_ptr<Collision> collision;

    // Перевіряємо, чи модель має геометрію
    bool hasValidGeometry = false;
    for (int i = 0; i < model.meshCount; i++)
    {
        if (model.meshes[i].vertices && model.meshes[i].vertexCount > 0)
        {
            hasValidGeometry = true;
            break;
        }
    }

    if (!hasValidGeometry)
    {
        // Фолбек AABB для моделей без геометрії
        TraceLog(LOG_WARNING, "Model '%s' has no valid geometry, creating fallback collision",
                 modelName.c_str());
        BoundingBox modelBounds = GetModelBoundingBox(model);
        Vector3 size = {modelBounds.max.x - modelBounds.min.x,
                        modelBounds.max.y - modelBounds.min.y,
                        modelBounds.max.z - modelBounds.min.z};
        Vector3 center = {(modelBounds.max.x + modelBounds.min.x) * 0.5f,
                          (modelBounds.max.y + modelBounds.min.y) * 0.5f,
                          (modelBounds.max.z + modelBounds.min.z) * 0.5f};

        // Collision expects half-size extents
        collision = std::make_shared<Collision>(center, Vector3Scale(size, 0.5f));
        collision->SetCollisionType(CollisionType::AABB_ONLY);
    }
    else
    {
        // Колізія з геометрії моделі
        collision = std::make_shared<Collision>();
        Model modelCopy = model; // копія для побудови колізії

        if (config)
            collision->BuildFromModel(&modelCopy, MatrixIdentity());

        // Встановлюємо тип колізії
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

    return collision;
}

Collision CollisionManager::CreatePreciseInstanceCollision(const Model &model, Vector3 position,
                                                           float scale,
                                                           const ModelFileConfig *config)
{
    Collision instanceCollision;

    // Масштаб -> (обертання) -> зсув
    Matrix transform = MatrixIdentity();
    transform = MatrixMultiply(transform, MatrixScale(scale, scale, scale));
    // Якщо є rotation:
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

Collision CollisionManager::CreatePreciseInstanceCollisionFromCached(const Collision &cachedCollision,
                                                                     Vector3 position, float scale)
{
    // Створюємо інстанс з уже наявних трикутників без повторного читання mesh'ів
    Collision instance;

    // Побудуємо трансформацію інстанса (масштаб -> зсув)
    Matrix transform = MatrixIdentity();
    transform = MatrixMultiply(transform, MatrixScale(scale, scale, scale));
    transform = MatrixMultiply(transform, MatrixTranslate(position.x, position.y, position.z));

    // Скопіюємо трикутники та застосуємо трансформацію
    const auto &tris = cachedCollision.GetTriangles();
    instance = Collision{};
    for (const auto &t : tris)
    {
        Vector3 v0 = Vector3Transform(t.V0(), transform);
        Vector3 v1 = Vector3Transform(t.V1(), transform);
        Vector3 v2 = Vector3Transform(t.V2(), transform);
        instance.AddTriangle(CollisionTriangle(v0, v1, v2));
    }

    // Оновимо AABB і зберемо BVH
    instance.UpdateAABBFromTriangles();
    instance.InitializeBVH();
    instance.SetCollisionType(CollisionType::BVH_ONLY);
    return instance;
}

Collision CollisionManager::CreateSimpleAABBInstanceCollision(const Collision &cachedCollision,
                                                              const Vector3 &position, float scale)
{
    // Створюємо чисту AABB без трикутників та BVH, щоб інстанс був суто AABB_ONLY
    Vector3 transformedCenter =
        Vector3Add(Vector3Scale(cachedCollision.GetCenter(), scale), position);
    Vector3 scaledSize = Vector3Scale(cachedCollision.GetSize(), scale);

    // Collision expects half-size extents
    Collision instanceCollision(transformedCenter, Vector3Scale(scaledSize, 0.5f));
    instanceCollision.SetCollisionType(CollisionType::AABB_ONLY);
    return instanceCollision;
}
