#include "core/Log.h"
#include "PlayerCollision.h"
#include "../core/player.h"
#include <cmath>

PlayerCollision::PlayerCollision(IPlayer *player) : m_player(player)
{
    UpdateBoundingBox();
}

void PlayerCollision::InitializeCollision()
{
    SetCollisionType(CollisionType::BVH_ONLY);
    UpdateBoundingBox();
    UpdateCollisionPoints();
}

void PlayerCollision::Update()
{
    UpdateBoundingBox();
    if (IsUsingBVH())
    {
        UpdateCollisionPoints();
    }
}

void PlayerCollision::UpdateCollisionPoints()
{
    Vector3 size = m_player->GetPlayerSize();
    Vector3 halfSize = Vector3Scale(size, 0.5f);

    // Use same center as bounding box for consistency
    Vector3 pos = m_player->GetPlayerPosition();
    Vector3 center = pos;
    center.y = pos.y + (-1.0f) + halfSize.y; // -1.0f is Player::MODEL_Y_OFFSET

    m_collisionPoints = {
        center,
        Vector3Subtract(center, {halfSize.x, 0, 0}), // left
        Vector3Add(center, {halfSize.x, 0, 0}),      // right
        Vector3Subtract(center, {0, 0, halfSize.z}), // front
        Vector3Add(center, {0, 0, halfSize.z}),      // back
        Vector3Subtract(center, {0, halfSize.y, 0}), // bottom
        Vector3Add(center, {0, halfSize.y, 0})       // top
    };
}

BoundingBox PlayerCollision::GetBoundingBox() const
{
    return m_boundingBox;
}

void PlayerCollision::UpdateBoundingBox()
{
    Vector3 pos = m_player->GetPlayerPosition();
    Vector3 size = m_player->GetPlayerSize();
    Vector3 halfSize = Vector3Scale(size, 0.5f);

    // Align collision center with visual model:
    // Model base is at pos.y + visualOffset
    // Collision center should be at visualBottom + halfSize.y
    float visualOffset = -1.0f; // matches Player::MODEL_Y_OFFSET
    Vector3 collisionCenter = pos;
    collisionCenter.y = pos.y + visualOffset + halfSize.y;

    // Sync base Collision AABB
    Collision::Update(collisionCenter, halfSize);
}

bool PlayerCollision::IsJumpCollision() const
{
    return m_isJumpCollision;
}

void PlayerCollision::SetJumpCollision(bool isJumpCollision)
{
    m_isJumpCollision = isJumpCollision;
}

void PlayerCollision::EnableBVHCollision(bool enable)
{
    SetCollisionType(enable ? CollisionType::BVH_ONLY : CollisionType::AABB_ONLY);
}

bool PlayerCollision::IsUsingBVH() const
{
    return GetCollisionType() == CollisionType::BVH_ONLY;
}

bool PlayerCollision::CheckCollisionWithBVH(const Collision &other, Vector3 &outResponse)
{
    if (!IsUsingBVH() || !other.IsUsingBVH())
    {
        return false;
    }

    bool hasCollision = false;
    Vector3 totalResponse = {0};

    const Vector3 directions[] = {{0, -1, 0}, {0, 1, 0}, {1, 0, 0},
                                  {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};

    for (const auto &point : m_collisionPoints)
    {
        for (const auto &dir : directions)
        {
            RayHit hit;
            if (other.RaycastBVH(point, dir, 1.0f, hit))
            {
                if (hit.hit)
                {
                    // Calculate proper collision response
                    // The response should push the player away from the collision surface
                    Vector3 response;
                    float penetration = hit.distance;

                    // Response should be along the surface normal to push the player OUT
                    response.x = hit.normal.x * penetration;
                    response.y = hit.normal.y * penetration;
                    response.z = hit.normal.z * penetration;

                    // Special handling for ground collision (when normal is mostly upward)
                    if (hit.normal.y > 0.7f && fabsf(hit.normal.x) < 0.3f &&
                        fabsf(hit.normal.z) < 0.3f)
                    {
                        // For ground collision, ensure the collision box is positioned correctly
                        // relative to the visual model's offset
                        float modelOffset = Player::MODEL_Y_OFFSET;
                        if (response.y > 0.0f) // Pushing upward (away from ground penetration)
                        {
                            // Adjust response to account for model offset
                            response.y += fabsf(modelOffset) * 0.1f;
                        }
                    }

                    // Only consider significant penetrations to avoid jitter
                    if (penetration > 0.01f)
                    {
                        if (!hasCollision || Vector3Length(response) < Vector3Length(totalResponse))
                        {
                            totalResponse = response;
                            hasCollision = true;
                        }
                    }
                }
            }
        }
    }

    if (hasCollision)
    {
        outResponse = totalResponse;
        CD_INFO("PlayerCollision::CheckCollisionWithBVH - Collision detected, response: (%.3f, "
                 "%.3f, %.3f)",
                 totalResponse.x, totalResponse.y, totalResponse.z);
    }
    else
    {
        CD_TRACE("PlayerCollision::CheckCollisionWithBVH - No collision detected");
    }

    return hasCollision;
}

