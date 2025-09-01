//
//

#ifndef COLLISIONMANAGER_H
#define COLLISIONMANAGER_H

#include "CollisionSystem.h"
#include <string>
#include <unordered_map>
#include <vector>

class Models;
//
// CollisionManager
// Manages all collision boxes in the game.
// Supports adding, clearing, and checking collisions.
//
class CollisionManager
{
public:
    CollisionManager() = default;

    // Initialize the collision system
    void Initialize() const;

    // Add a new collider to the manager
    void AddCollider(Collision &collider);

    // Remove all colliders
    void ClearColliders();

    // Check if the player collision intersects with any collider
    [[nodiscard]] bool CheckCollision(const Collision &playerCollision) const;

    // Check collision and provide collision response vector
    [[nodiscard]] bool CheckCollision(const Collision &playerCollision, Vector3 &response) const;

    // Get all colliders
    [[nodiscard]] const std::vector<Collision> &GetColliders() const;
    // Create collision for models automatically
    void CreateAutoCollisionsFromModels(Models &models);
    // Helper function to create cache key
    [[nodiscard]] std::string MakeCollisionCacheKey(const std::string &modelName,
                                                    float scale) const;
    bool CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                  Vector3 position, float scale, const Models &models);
    std::shared_ptr<Collision> CreateBaseCollision(const Model &model, const std::string &modelName,
                                                   const ModelFileConfig *config,
                                                   bool needsPreciseCollision);
    Collision CreatePreciseInstanceCollision(const Model &model, Vector3 position, float scale,
                                             const ModelFileConfig *config);

    Collision CreateSimpleInstanceCollision(const Collision &cachedCollision, Vector3 position,
                                            float scale);

private:
    std::vector<Collision> m_collisions; // List of all collision boxes

    // Collision cache to prevent rebuilding octrees for same models
    std::unordered_map<std::string, std::shared_ptr<Collision>> m_collisionCache;

    // Counter for precise collisions per model to limit memory usage
    std::unordered_map<std::string, int> m_preciseCollisionCount;
    static constexpr int MAX_PRECISE_COLLISIONS_PER_MODEL = 50; // Limit precise collisions
};

#endif // COLLISIONMANAGER_H
