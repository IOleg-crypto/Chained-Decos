#include "PlayerRenderable.h"
#include "Player.h"

PlayerRenderable::PlayerRenderable(Player* player) : m_player(player)
{
}

void PlayerRenderable::Update(CollisionManager& collisionManager)
{
    m_player->UpdateImpl(collisionManager);
}

Vector3 PlayerRenderable::GetPosition() const
{
    return m_player->GetPlayerPosition();
}

BoundingBox PlayerRenderable::GetBoundingBox() const
{
    return m_player->GetPlayerBoundingBox();
}

float PlayerRenderable::GetRotationY() const
{
    return m_player->GetRotationY();
}

void PlayerRenderable::UpdateCollision()
{
    m_player->UpdatePlayerCollision();
}

const Collision& PlayerRenderable::GetCollision() const
{
    return m_player->GetCollision();
}

Camera PlayerRenderable::GetCamera() const
{
    return m_player->GetCameraController()->GetCamera();
}

bool PlayerRenderable::IsGrounded() const
{
    return m_player->GetPhysics().IsGrounded();
}

float PlayerRenderable::GetVelocityY() const
{
    return m_player->GetPhysics().GetVelocity().y;
}

