#include "PlayerRenderable.h"
#include "core/interfaces/IPlayer.h"

void PlayerRenderable::Update(IPlayer &player, CollisionManager &collisionManager)
{
    // Player-specific update logic can be called through IPlayer interface
    player.Update(0.0f); // or use collisionManager
}

Vector3 PlayerRenderable::GetPosition(const IPlayer &player) const
{
    return player.GetPosition();
}

BoundingBox PlayerRenderable::GetBoundingBox(const IPlayer &player) const
{
    return player.GetPlayerBoundingBox();
}

float PlayerRenderable::GetRotationY(const IPlayer &player) const
{
    return player.GetRotationY();
}

void PlayerRenderable::UpdateCollision(IPlayer &player)
{
    player.SyncCollision();
}

const Collision &PlayerRenderable::GetCollision(const IPlayer &player) const
{
    return player.GetCollision();
}

Camera PlayerRenderable::GetCamera(IPlayer &player)
{
    return player.GetCamera();
}

bool PlayerRenderable::IsGrounded(const IPlayer &player) const
{
    return player.GetPhysics().IsGrounded();
}

float PlayerRenderable::GetVelocityY(const IPlayer &player) const
{
    return player.GetPhysics().GetVelocity().y;
}

// Legacy interface overrides - delegate to stored player
void PlayerRenderable::Update(CollisionManager &collisionManager)
{
    if (m_player)
        Update(*m_player, collisionManager);
}

Vector3 PlayerRenderable::GetPosition() const
{
    return m_player ? GetPosition(*m_player) : Vector3{0, 0, 0};
}

BoundingBox PlayerRenderable::GetBoundingBox() const
{
    return m_player ? GetBoundingBox(*m_player) : BoundingBox{};
}

float PlayerRenderable::GetRotationY() const
{
    return m_player ? GetRotationY(*m_player) : 0.0f;
}

void PlayerRenderable::UpdateCollision()
{
    if (m_player)
        UpdateCollision(*m_player);
}

const Collision &PlayerRenderable::GetCollision() const
{
    static Collision empty;
    return m_player ? GetCollision(*m_player) : empty;
}

Camera PlayerRenderable::GetCamera() const
{
    return m_player ? m_player->GetCamera() : Camera{};
}

bool PlayerRenderable::IsGrounded() const
{
    return m_player ? IsGrounded(*m_player) : false;
}

float PlayerRenderable::GetVelocityY() const
{
    return m_player ? GetVelocityY(*m_player) : 0.0f;
}
void PlayerRenderable::SetPlayer(IPlayer *player)
{
    m_player = player;
}






