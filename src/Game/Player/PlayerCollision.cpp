#include "Player.h"
#include "PlayerCollision.h"

PlayerCollision::PlayerCollision(Player *player)
    : m_player(player)
{
    UpdateBoundingBox();
    m_collision.Update(m_player->GetPlayerPosition(), m_player->GetPlayerSize());
}

void PlayerCollision::Update()
{
    // Синхронізуємо колізію з позицією гравця
    Vector3 position = m_player->GetPlayerPosition();
    Vector3 size = m_player->GetPlayerSize();

    // Collision system очікує повний розмір
    m_collision.Update(position, size);

    // Оновлюємо AABB для швидких перевірок
    UpdateBoundingBox();
}

const Collision &PlayerCollision::GetCollision() const
{
    return m_collision;
}

BoundingBox PlayerCollision::GetBoundingBox() const
{
    return m_boundingBox;
}

void PlayerCollision::UpdateBoundingBox()
{
    Vector3 position = m_player->GetPlayerPosition();
    Vector3 size = m_player->GetPlayerSize();

    // Мінімальна та максимальна точка AABB
    m_boundingBox.min = Vector3Subtract(position, Vector3Scale(size, 0.5f));
    m_boundingBox.max = Vector3Add(position, Vector3Scale(size, 0.5f));
}

bool PlayerCollision::IsJumpCollision() const
{
    return m_isJumpCollision;
}

void PlayerCollision::SetJumpCollision(bool isJumpCollision)
{
    m_isJumpCollision = isJumpCollision;
}
