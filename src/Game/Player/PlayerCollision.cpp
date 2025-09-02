#include "Player.h"
#include "PlayerCollision.h"

PlayerCollision::PlayerCollision(Player *player) : m_player(player), m_isJumpCollision(false)
{
    UpdateBoundingBox();
}

void PlayerCollision::Update()
{
    Vector3 position = m_player->GetPlayerPosition();
    Vector3 size = m_player->GetPlayerSize();
    // Collision system expects half-extents for size
    m_collision.Update(position, Vector3Scale(size, 2.5f));
}

const Collision &PlayerCollision::GetCollision() const { return m_collision; }

BoundingBox PlayerCollision::GetBoundingBox() const { return m_boundingBox; }

void PlayerCollision::UpdateBoundingBox()
{
    Vector3 position = m_player->GetPlayerPosition();
    Vector3 size = m_player->GetPlayerSize();

    m_boundingBox.min = Vector3Subtract(position, Vector3Scale(size, 0.5f));
    m_boundingBox.max = Vector3Add(position, Vector3Scale(size, 0.5f));
}

bool PlayerCollision::IsJumpCollision() const { return m_isJumpCollision; }

void PlayerCollision::SetJumpCollision(bool isJumpCollision)
{
    m_isJumpCollision = isJumpCollision;
}