#ifndef COLLISIONMANAGER_H
#define COLLISIONMANAGER_H

#include <Collision/CollisionSystem.h>
#include <vector>

class CollisionManager
{
private:
      std::vector<Collision> m_collisions; // List of all collisions in the game
public:
      CollisionManager() = default;
      void AddCollider(Collision &collider);
      void ClearColliders();
      bool CheckCollision(const Collision &playerCollision) const;
      bool CheckCollision(const Collision &playerCollision, Vector3 &response) const;
      const std::vector<Collision>& GetColliders() const { return m_collisions; }
};

#endif /* COLLISIONMANAGER_H */
