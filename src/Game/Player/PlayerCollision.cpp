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
    Vector3 pos = m_player->GetPlayerPosition();
    Vector3 halfSize = Vector3Scale(m_player->GetPlayerSize(), 0.5f);

    m_collisionPoints = {
        pos,
        Vector3Subtract(pos, {halfSize.x, 0, 0}),        // left
        Vector3Add(pos, {halfSize.x, 0, 0}),             // right
        Vector3Subtract(pos, {0, 0, halfSize.z}),        // front
        Vector3Add(pos, {0, 0, halfSize.z}),             // back
        Vector3Subtract(pos, {0, halfSize.y, 0}),        // bottom
        Vector3Add(pos, {0, halfSize.y, 0})              // top
    };
}

BoundingBox PlayerCollision::GetBoundingBox() const {
    return m_boundingBox;
}

void PlayerCollision::UpdateBoundingBox() {
    Vector3 pos = m_player->GetPlayerPosition();
    Vector3 halfSize = Vector3Scale(m_player->GetPlayerSize(), 0.5f);

    m_boundingBox.min = Vector3Subtract(pos, halfSize);
    m_boundingBox.max = Vector3Add(pos, halfSize);

    // Sync base Collision AABB
    Collision::Update(pos, halfSize);
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
                    // Calculate proper collision response
                    // The response should push the player away from the collision surface
                    Vector3 response;
                    float penetration = hit.distance;

                    // Response should be opposite to the surface normal, not ray direction
                    response.x = -hit.normal.x * penetration;
                    response.y = -hit.normal.y * penetration;
                    response.z = -hit.normal.z * penetration;

                    // Only consider significant penetrations to avoid jitter
                    if (penetration > 0.01f) {
                        if (!hasCollision || Vector3Length(response) < Vector3Length(totalResponse)) {
                            totalResponse = response;
                            hasCollision = true;
                        }
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
