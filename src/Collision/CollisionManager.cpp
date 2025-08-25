#include <Collision/CollisionManager.h>
#include <Model/Model.h>
#include <algorithm>
#include <compare>
#include <raylib.h>
#include <set>
#include <string>
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
        TraceLog(LOG_INFO, "No collisions");
        return false;
    }

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

        // 1. Знаходимо overlap по кожній осі
        float overlapX = fminf(playerMax.x, colliderMax.x) - fmaxf(playerMin.x, colliderMin.x);
        float overlapY = fminf(playerMax.y, colliderMax.y) - fmaxf(playerMin.y, colliderMin.y);
        float overlapZ = fminf(playerMax.z, colliderMax.z) - fmaxf(playerMin.z, colliderMin.z);

        if (overlapX <= 0 || overlapY <= 0 || overlapZ <= 0)
            continue;

        // 2. Визначаємо найменший overlap
        float minOverlap = fabsf(overlapX);
        int axis = 0; // 0 - x, 1 - y, 2 - z

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

        // 3. Формуємо MTV тільки по цій осі
        Vector3 mtv = {0, 0, 0};
        if (axis == 0)
        {
            mtv.x = ((playerMin.x + playerMax.x) / 2 < (colliderMin.x + colliderMax.x) / 2)
                        ? -minOverlap
                        : minOverlap;
        }
        else if (axis == 1)
        {
            mtv.y = ((playerMin.y + playerMax.y) / 2 < (colliderMin.y + colliderMax.y) / 2)
                        ? -minOverlap
                        : minOverlap;
        }
        else
        {
            mtv.z = ((playerMin.z + playerMax.z) / 2 < (colliderMin.z + colliderMax.z) / 2)
                        ? -minOverlap
                        : minOverlap;
        }

        // Пом’якшення відштовхування
        const float RESPONSE_FACTOR = 0.5f;
        mtv = Vector3Scale(mtv, RESPONSE_FACTOR);

        // Застосовуємо response послідовно
        response = Vector3Add(response, mtv);
    }

    return collided;
}

void CollisionManager::CreateAutoCollisionsFromModels(Models &models)
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
                                                Vector3 position, float scale, const Models &models)
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
             config->collisionPrecision == CollisionPrecision::OCTREE_ONLY ||
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

    // Determine if we need precise collision for this instance
    bool usePreciseForInstance =
        needsPreciseCollision &&
        (cachedType == CollisionType::OCTREE_ONLY || cachedType == CollisionType::IMPROVED_AABB ||
         cachedType == CollisionType::TRIANGLE_PRECISE);

    if (usePreciseForInstance)
    {
        // Check if we've reached the limit for precise collisions for this model
        int &preciseCount = m_preciseCollisionCount[modelName];

        if (preciseCount < MAX_PRECISE_COLLISIONS_PER_MODEL) // Only use precise collision for "arc" model
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
            instanceCollision = CreateSimpleInstanceCollision(*cachedCollision, position, scale);
        }
    }
    else
    {
        // Create simple AABB collision
        instanceCollision = CreateSimpleInstanceCollision(*cachedCollision, position, scale);
    }

    // Add the instance collision to collision manager
    size_t beforeCount = GetColliders().size();
    AddCollider(instanceCollision);
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

const std::vector<Collision> &CollisionManager::GetColliders() const { return m_collisions; }

// Helper method to create base collision for caching
std::shared_ptr<Collision> CollisionManager::CreateBaseCollision(const Model &model,
                                                                 const std::string &modelName,
                                                                 const ModelFileConfig *config,
                                                                 bool needsPreciseCollision)
{
    std::shared_ptr<Collision> collision;

    // Check if model has valid geometry
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
        // Create fallback AABB collision for models without geometry
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
    }
    else
    {
        // Create collision from model geometry
        collision = std::make_shared<Collision>();
        Model modelCopy = model; // Make a copy for collision building

        // Build base collision at origin without transformation
        if (config)
        {
            collision->BuildFromModelConfig(&modelCopy, *config, MatrixIdentity());
        }
        else
        {
            collision->BuildFromModel(&modelCopy, MatrixIdentity());
        }

        // Set correct collision type for precise configs
        if (needsPreciseCollision && config)
        {
            CollisionType targetType = CollisionType::HYBRID_AUTO;

            if (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE)
            {
                targetType = CollisionType::TRIANGLE_PRECISE;
            }
            else if (config->collisionPrecision == CollisionPrecision::OCTREE_ONLY)
            {
                targetType = CollisionType::OCTREE_ONLY;
            }
            else if (config->collisionPrecision == CollisionPrecision::IMPROVED_AABB)
            {
                targetType = CollisionType::IMPROVED_AABB;
            }

            collision->SetCollisionType(targetType);
        }
    }

    return collision;
}

// Helper method to create precise instance collision
Collision CollisionManager::CreatePreciseInstanceCollision(const Model &model, Vector3 position,
                                                           float scale,
                                                           const ModelFileConfig *config)
{
    Collision instanceCollision;

    // Create transformation matrix with both scale and position
    Matrix transform = MatrixMultiply(MatrixScale(scale, scale, scale),
                                      MatrixTranslate(position.x, position.y, position.z));

    // Create a copy of the model for collision building
    Model modelCopy = model;

    // Build collision with full transformation
    if (config)
    {
        instanceCollision.BuildFromModelConfig(&modelCopy, *config, transform);
    }
    else
    {
        instanceCollision.BuildFromModel(&modelCopy, transform);
    }

    // Use OCTREE_ONLY for models (more stable than TRIANGLE_PRECISE)
    instanceCollision.SetCollisionType(CollisionType::OCTREE_ONLY);

    TraceLog(LOG_INFO, "Built OCTREE collision for instance at (%.2f, %.2f, %.2f)", position.x,
             position.y, position.z);

    return instanceCollision;
}

// Helper method to create simple AABB instance collision
Collision CollisionManager::CreateSimpleInstanceCollision(const Collision &cachedCollision,
                                                          Vector3 position, float scale)
{
    Collision instanceCollision = cachedCollision;

    // Apply transformation to center and scale
    Vector3 cachedCenter = cachedCollision.GetCenter();
    Vector3 cachedSize = cachedCollision.GetSize();
    Vector3 transformedCenter = Vector3Add(Vector3Scale(cachedCenter, scale), position);
    Vector3 scaledSize = Vector3Scale(cachedSize, scale);

    instanceCollision.Update(transformedCenter, scaledSize);

    TraceLog(LOG_INFO, "Created AABB collision for instance at (%.2f, %.2f, %.2f)", position.x,
             position.y, position.z);

    return instanceCollision;
}