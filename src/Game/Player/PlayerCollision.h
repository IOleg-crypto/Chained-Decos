#ifndef PLAYER_COLLISION_H
#define PLAYER_COLLISION_H

#include <raylib.h>
#include <Collision/CollisionSystem.h>
#include <vector>
#include "Player.h"

// PlayerCollision: handles collision detection and response
class PlayerCollision : public Collision {
public:
    explicit PlayerCollision(Player* player);
    ~PlayerCollision() = default;
    
    void InitializeCollision();
    void Update();
    BoundingBox GetBoundingBox() const;
    void UpdateBoundingBox();
    bool IsJumpCollision() const;
    void SetJumpCollision(bool isJumpCollision);
    
    // BVH collision methods
    void EnableBVHCollision(bool enable);
    bool IsUsingBVH() const;
    bool CheckCollisionWithBVH(const Collision& other, Vector3& outResponse);

private:
    Player* m_player;
    BoundingBox m_boundingBox{};
    bool m_isJumpCollision = false;
    std::vector<Vector3> m_collisionPoints;
    
    void UpdateCollisionPoints();
};

#endif // PLAYER_COLLISION_H


