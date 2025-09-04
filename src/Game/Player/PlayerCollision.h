
#ifndef PLAYER_COLLISION_H
#define PLAYER_COLLISION_H

#include <raylib.h>
#include <Collision/CollisionSystem.h>

// Forward declarations
class Player;

// PlayerCollision: handles collision detection and response
class PlayerCollision
{
public:
    PlayerCollision(Player* player);
    ~PlayerCollision() = default;
    
    // Update collision data
    void Update();
    
    // Get collision data
    const Collision& GetCollision() const;
    
    // Get player bounding box
    BoundingBox GetBoundingBox() const;
    
    // Update bounding box
    void UpdateBoundingBox();
    
    // Check if player is in jump collision
    bool IsJumpCollision() const;
    
    // Set jump collision flag
    void SetJumpCollision(bool isJumpCollision);
    
private:
    Player* m_player;
    Collision m_collision;
    BoundingBox m_boundingBox{};
    bool m_isJumpCollision = false;
};

#endif // PLAYER_COLLISION_H


