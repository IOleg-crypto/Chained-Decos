#include "Player.h"
#include "PlayerCollision.h"

PlayerCollision::PlayerCollision(Player* player) 
    : m_player(player) {
    UpdateBoundingBox();
}

void PlayerCollision::InitializeCollision() {
    SetCollisionType(CollisionType::BVH_ONLY);
    UpdateBoundingBox();
    UpdateCollisionPoints();
}

void PlayerCollision::Update() {
    UpdateBoundingBox();
    if (IsUsingBVH()) {
        UpdateCollisionPoints();
    }
}

void PlayerCollision::UpdateCollisionPoints() {
    Vector3 position = m_player->GetPlayerPosition();
    Vector3 size = m_player->GetPlayerSize();
    
    float dx = size.x * 0.5f;
    float dy = size.y * 0.5f;
    float dz = size.z * 0.5f;
    
    m_collisionPoints = {
        position,                                        // center
        {position.x - dx, position.y, position.z},      // left
        {position.x + dx, position.y, position.z},      // right
        {position.x, position.y, position.z - dz},      // front
        {position.x, position.y, position.z + dz},      // back
        {position.x, position.y - dy, position.z},      // bottom
        {position.x, position.y + dy, position.z}       // top
    };
}

BoundingBox PlayerCollision::GetBoundingBox() const {
    return m_boundingBox;
}

void PlayerCollision::UpdateBoundingBox() {
    Vector3 position = m_player->GetPlayerPosition();
    Vector3 size = m_player->GetPlayerSize();
    
    m_boundingBox.min = Vector3Subtract(position, Vector3Scale(size, 0.5f));
    m_boundingBox.max = Vector3Add(position, Vector3Scale(size, 0.5f));

    // Keep base Collision AABB in sync with the player bounding box
    // so collision queries use the latest extents
    Collision::Update(
        Vector3Scale(Vector3Add(m_boundingBox.min, m_boundingBox.max), 0.5f),
        Vector3Scale(Vector3Subtract(m_boundingBox.max, m_boundingBox.min), 0.5f)
    );
}

bool PlayerCollision::IsJumpCollision() const {
    return m_isJumpCollision;
}

void PlayerCollision::SetJumpCollision(bool isJumpCollision) {
    m_isJumpCollision = isJumpCollision;
}

void PlayerCollision::EnableBVHCollision(bool enable) {
    SetCollisionType(enable ? CollisionType::BVH_ONLY : CollisionType::AABB_ONLY);
}

bool PlayerCollision::IsUsingBVH() const {
    return GetCollisionType() == CollisionType::BVH_ONLY;
}

bool PlayerCollision::CheckCollisionWithBVH(const Collision& other, Vector3& outResponse) {
    if (!IsUsingBVH() || !other.IsUsingBVH()) {
        return false;
    }

    bool hasCollision = false;
    Vector3 totalResponse = {0};
    
    const Vector3 directions[] = {
        {0, -1, 0},  
        {0, 1, 0},   
        {1, 0, 0},   
        {-1, 0, 0},  
        {0, 0, 1},   
        {0, 0, -1}   
    };
    
    for (const auto& point : m_collisionPoints) {
        for (const auto& dir : directions) {
            RayHit hit;
            if (other.RaycastBVH(point, dir, 2.0f, hit)) {
                if (hit.hit) {
                    
                    Vector3 response;
                    float penetration = 2.0f - hit.distance;
                    response.x = dir.x * penetration;
                    response.y = dir.y * penetration;
                    response.z = dir.z * penetration;
                    
         
                    if (!hasCollision || Vector3Length(response) < Vector3Length(totalResponse)) {
                        totalResponse = response;
                        hasCollision = true;
                    }
                }
            }
        }
    }
    
    if (hasCollision) {
        outResponse = totalResponse;
    }
    
    return hasCollision;
}
