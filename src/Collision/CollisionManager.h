//
//

#ifndef COLLISIONMANAGER_H
#define COLLISIONMANAGER_H

#include <Collision/CollisionSystem.h>
#include <vector>

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
    void Initialize();

    // Add a new collider to the manager
    void AddCollider(Collision &collider);

    // Remove all colliders
    void ClearColliders();

    // Check if the player collision intersects with any collider
    [[nodiscard]] bool CheckCollision(const Collision &playerCollision) const;

    // Check collision and provide collision response vector
    [[nodiscard]] bool CheckCollision(const Collision &playerCollision, Vector3 &response) const;

    // Get all colliders
    [[nodiscard]] const std::vector<Collision> &GetColliders() const { return m_collisions; }

private:
    std::vector<Collision> m_collisions; // List of all collision boxes
};

#endif // COLLISIONMANAGER_H
