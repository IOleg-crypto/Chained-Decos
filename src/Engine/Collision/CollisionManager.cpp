#include <CollisionManager.h>
#include <CollisionSystem.h>
#include <Model/Model.h>
#include <algorithm>
#include <compare>
#include <raylib.h>
#include <set>
#include <string>
#include <vector>

void CollisionManager::Initialize() const
{
    for (auto &collider : m_collisions)
    {
        if (collider->GetCollisionType() == CollisionType::BVH_ONLY ||
            collider->GetCollisionType() == CollisionType::TRIANGLE_PRECISE)
        {
            collider->InitializeBVH();
        }
    }

    TraceLog(LOG_INFO, "CollisionManager initialized with %zu colliders", m_collisions.size());
}

void CollisionManager::AddCollider(Collision &&collider)
{
    m_collisions.push_back(std::make_unique<Collision>(std::move(collider)));

    if (m_collisions.back()->GetCollisionType() == CollisionType::BVH_ONLY ||
        m_collisions.back()->GetCollisionType() == CollisionType::TRIANGLE_PRECISE)
    {
        m_collisions.back()->InitializeBVH();
    }

    TraceLog(LOG_INFO, "Added collider, total count: %zu", m_collisions.size());
}

void CollisionManager::AddColliderRef(Collision* collider)
{
    if (!collider) return;
    
    m_collisions.push_back(std::unique_ptr<Collision>(collider));
    
    if (collider->GetCollisionType() == CollisionType::BVH_ONLY ||
        collider->GetCollisionType() == CollisionType::TRIANGLE_PRECISE)
    {
        collider->InitializeBVH();
    }
    
    TraceLog(LOG_INFO, "Added collider reference, total count: %zu", m_collisions.size());
}

void CollisionManager::ClearColliders() { m_collisions.clear(); }

bool CollisionManager::CheckCollision(const Collision &playerCollision) const
{
    if (m_collisions.empty())
        return false;

    for (const auto &collider : m_collisions)
    {
        bool hit = false;

        if (collider->IsUsingBVH())
            hit = playerCollision.IntersectsBVH(*collider);
        else
            hit = playerCollision.Intersects(*collider);

        if (hit)
            return true;
    }
    return false;
}
bool CollisionManager::CheckCollision(const Collision &playerCollision, Vector3 &response) const
{
    if (m_collisions.empty())
        return false;
    const Vector3 playerMin = playerCollision.GetMin();
    const Vector3 playerMax = playerCollision.GetMax();
    const Vector3 playerCenter = {(playerMin.x + playerMax.x) * 0.5f,
                                  (playerMin.y + playerMax.y) * 0.5f,
                                  (playerMin.z + playerMax.z) * 0.5f};
    bool collided = false;
    response = (Vector3){0, 0, 0};
    bool hasBest = false;
    Vector3 bestMTV = {0, 0, 0};
    float bestLenSq = FLT_MAX;
    Vector3 groundMTV = {0, 0, 0};
    bool hasGroundMTV = false;
    for (const auto &collider : m_collisions)
    {
        bool hit = collider->IsUsingBVH() ? playerCollision.IntersectsBVH(*collider)
                                          : playerCollision.Intersects(*collider);
        if (!hit)
            continue;
        collided = true;

        // --- Додаємо raycast вниз по BVH ---
        if (collider->IsUsingBVH()) {
            Vector3 origin = playerCenter;
            Vector3 dir = {0.0f, -1.0f, 0.0f};
            float rayMax = (playerMax.y - playerMin.y) + 1.0f;
            RayHit rayHit;
            if (collider->RaycastBVH(origin, dir, rayMax, rayHit) && rayHit.hit) {
                float deltaUp = rayHit.position.y - playerMin.y;
                if (deltaUp > 0.0f && deltaUp < rayMax) {
                    Vector3 refined = {0.0f, deltaUp, 0.0f};
                    if (!hasGroundMTV || fabsf(refined.y) < fabsf(groundMTV.y)) {
                        groundMTV = refined;
                        hasGroundMTV = true;
                    }
                }
            }
        }
        // --- залишаємо старий AABB для fallback ---
        const Vector3 colliderMin = collider->GetMin();
        const Vector3 colliderMax = collider->GetMax();
        const Vector3 colliderCenter = {(colliderMin.x + colliderMax.x) * 0.5f,
                                        (colliderMin.y + colliderMax.y) * 0.5f,
                                        (colliderMin.z + colliderMax.z) * 0.5f};
        const float overlapX =
            fminf(playerMax.x, colliderMax.x) - fmaxf(playerMin.x, colliderMin.x);
        const float overlapY =
            fminf(playerMax.y, colliderMax.y) - fmaxf(playerMin.y, colliderMin.y);
        const float overlapZ =
            fminf(playerMax.z, colliderMax.z) - fmaxf(playerMin.z, colliderMin.z);
        if (overlapX <= 0 || overlapY <= 0 || overlapZ <= 0)
            continue;
        float minOverlap = fabsf(overlapX);
        int axis = 0;
        if (fabsf(overlapY) < minOverlap)
        {
            minOverlap = fabsf(overlapY);
            axis = 1;
        }
        if (fabsf(overlapZ) < minOverlap)
        {
            minOverlap = fabsf(overlapZ);
            axis = 2;
        }
        Vector3 mtv = {0, 0, 0};
        switch (axis)
        {
        case 0:
            mtv.x = (playerCenter.x < colliderCenter.x) ? -minOverlap : minOverlap;
            break;
        case 1:
            mtv.y = (playerCenter.y < colliderCenter.y) ? -minOverlap : minOverlap;
            break;
        case 2:
            mtv.z = (playerCenter.z < colliderCenter.z) ? -minOverlap : minOverlap;
            break;
        default:
            break;
        }
        if (axis == 1 && mtv.y > 0 && (playerCenter.y - colliderCenter.y) >= 0.1f)
        {
            if (!hasGroundMTV || fabsf(mtv.y) < fabsf(groundMTV.y))
            {
                groundMTV = mtv;
                hasGroundMTV = true;
            }
        }
        else
        {
            float lenSq = mtv.x * mtv.x + mtv.y * mtv.y + mtv.z * mtv.z;
            if (!hasBest || lenSq < bestLenSq)
            {
                bestLenSq = lenSq;
                bestMTV = mtv;
                hasBest = true;
            }
        }
    }
    if (hasGroundMTV)
    {
        response = groundMTV;
        return true;
    }
    if (hasBest)
    {
        response = bestMTV;
        return true;
    }
    return collided;
}

void CollisionManager::CreateAutoCollisionsFromModels(ModelLoader &models)
{
    TraceLog(LOG_INFO, "Starting automatic collision generation for all models...");

    // Get all available models
    auto availableModels = models.GetAvailableModels();
    TraceLog(LOG_INFO, "Found %zu models to check", availableModels.size());

    // Track processed models to avoid duplication
    std::set<std::string> processedModels;
    int collisionsCreated = 0;
    constexpr size_t MAX_COLLISION_INSTANCES = 3; // Reduced safety limit for better performance

    // Process each model
    for (const auto &modelName : availableModels)
    {
        // Skip if already processed
        if (processedModels.contains(modelName))
        {
            continue;
        }

        processedModels.insert(modelName);
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
                    collisionsCreated++;
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
                        collisionsCreated++;
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
             "Automatic collision generation complete. Created %d collisions from %zu models",
             collisionsCreated, availableModels.size());
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

    // --- STEP 1: Get model configuration and determine collision type ---
    const ModelFileConfig *config = models.GetModelConfig(modelName);
    bool needsPreciseCollision = false;

    if (config)
    {
        // Check if model needs precise collision based on its configuration
        needsPreciseCollision =
            (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE ||
             config->collisionPrecision == CollisionPrecision::BVH_ONLY ||
             config->collisionPrecision == CollisionPrecision::IMPROVED_AABB ||
             config->collisionPrecision == CollisionPrecision::AUTO);
    }
    else
    {
        TraceLog(LOG_WARNING, "No config found for model '%s'", modelName.c_str());
    }

    // --- STEP 2: Check collision cache or create new collision ---
    std::string cacheKey = MakeCollisionCacheKey(modelName, scale);
    std::shared_ptr<Collision> cachedCollision;

    // Try to find in cache first
    auto cacheIt = m_collisionCache.find(cacheKey);
    if (cacheIt != m_collisionCache.end())
    {
        cachedCollision = cacheIt->second;
        TraceLog(LOG_INFO, "Using cached collision for '%s'", cacheKey.c_str());
    }
    else
    {
        // Need to build a new collision
        cachedCollision = CreateBaseCollision(model, modelName, config, needsPreciseCollision);
        m_collisionCache[cacheKey] = cachedCollision;
    }

    // --- STEP 3: Create instance collision from cached collision ---
    Collision instanceCollision;
    CollisionType cachedType = cachedCollision->GetCollisionType();
    TraceLog(LOG_INFO , "Cached collision type for '%s' is %d", modelName.c_str(),
             static_cast<int>(cachedType));

    bool usePreciseForInstance =
        needsPreciseCollision &&
        (cachedType == CollisionType::BVH_ONLY || cachedType == CollisionType::TRIANGLE_PRECISE);

    if (usePreciseForInstance)
    {
        // Check if we've reached the limit for precise collisions for this model
        int &preciseCount = m_preciseCollisionCount[modelName];

        if (preciseCount <
            MAX_PRECISE_COLLISIONS_PER_MODEL) // Only use precise collision for "arc" model
        {
            // Create precise collision with full transformation
            instanceCollision = CreatePreciseInstanceCollision(model, position, scale, config);
            preciseCount++;
        }
        else
        {
            // Fallback to AABB if we reached the limit
            TraceLog(LOG_WARNING,
                     "Reached limit of %d precise collisions for model '%s', using AABB",
                     MAX_PRECISE_COLLISIONS_PER_MODEL, modelName.c_str());
            instanceCollision =
                CreateSimpleAABBInstanceCollision(*cachedCollision, position, scale);
        }
    }
    else
    {
        // Create simple AABB collision
        instanceCollision = CreateSimpleAABBInstanceCollision(*cachedCollision, position, scale);
    }

    // Add the instance collision to collision manager
    size_t beforeCount = GetColliders().size();
    AddCollider(std::move(instanceCollision));
    size_t afterCount = GetColliders().size();

    if (afterCount > beforeCount)
    {
        TraceLog(LOG_INFO,
                 "Successfully created instance collision for '%s', collider count: %zu -> %zu",
                 modelName.c_str(), beforeCount, afterCount);
        return true;
    }
    else
    {
        TraceLog(LOG_ERROR, "FAILED to add collision for '%s', collider count unchanged: %zu",
                 modelName.c_str(), beforeCount);
        return false;
    }
}

const std::vector<std::unique_ptr<Collision>> &CollisionManager::GetColliders() const
{
    return m_collisions;
}

bool CollisionManager::RaycastDown(const Vector3 &origin, float maxDistance, float &hitDistance,
                                   Vector3 &hitPoint, Vector3 &hitNormal) const
{
    bool anyHit = false;
    float bestDist = maxDistance;
    Vector3 bestPoint = {0};
    Vector3 bestNormal = {0};

    Vector3 dir = {0.0f, -1.0f, 0.0f}; // вниз

    for (const auto &collider : m_collisions)
    {
        if (!collider->IsUsingBVH())
            continue;

        RayHit hit;
        hit.hit = false;
        hit.distance = maxDistance;

        if (collider->RaycastBVH(origin, dir, maxDistance, hit))
        {
            if (hit.distance < bestDist)
            {
                bestDist = hit.distance;
                bestPoint = hit.position;
                bestNormal = hit.normal;
                anyHit = true;
            }
        }
    }

    if (anyHit)
    {
        hitDistance = bestDist;
        hitPoint = bestPoint;
        hitNormal = bestNormal;
        return true;
    }
    return false;
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

        collision = std::make_shared<Collision>(center, size);
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

Collision CollisionManager::CreateSimpleAABBInstanceCollision(const Collision &cachedCollision,
                                                              const Vector3 &position, float scale)
{
    Collision instanceCollision = cachedCollision;  // тут копіюються трикутники, але BVH губиться
    Vector3 transformedCenter =
        Vector3Add(Vector3Scale(cachedCollision.GetCenter(), scale), position);
    Vector3 scaledSize = Vector3Scale(cachedCollision.GetSize(), scale);

    instanceCollision.Update(transformedCenter, scaledSize);  // оновлюється тільки AABB
    instanceCollision.SetCollisionType(CollisionType::AABB_ONLY);  // явно встановлюється AABB_ONLY

    return instanceCollision;
}
