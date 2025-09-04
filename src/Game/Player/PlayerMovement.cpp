#ifndef PLAYERMOVEMENT_H
#define PLAYERMOVEMENT_H

#include "PlayerMovement.h"
#include "Player.h"
#include <cmath>
#include <raylib.h>

PlayerMovement::PlayerMovement(Player *player)
    : m_player(player), m_position(Player::DEFAULT_SPAWN_POSITION), m_rotationY(0.0f),
      m_walkSpeed(11.0f)
{
    m_physics.SetGroundLevel(false);
    m_physics.SetVelocity({0, 0, 0});
}

void PlayerMovement::Move(const Vector3 &moveVector)
{
    m_position = Vector3Add(m_position, moveVector);
}

void PlayerMovement::SetPosition(const Vector3 &pos)
{
    m_position = pos;
    m_player->SyncCollision();
}

Vector3 PlayerMovement::GetPosition() const { return m_position; }
float PlayerMovement::GetRotationY() const { return m_rotationY; }
void PlayerMovement::SetRotationY(float rotation) { m_rotationY = rotation; }

float PlayerMovement::GetSpeed() { return m_walkSpeed; }
void PlayerMovement::SetSpeed(float speed) { m_walkSpeed = speed; }

PhysicsComponent &PlayerMovement::GetPhysics() { return m_physics; }
const PhysicsComponent &PlayerMovement::GetPhysics() const { return m_physics; }

void PlayerMovement::SetCollisionManager(const CollisionManager *collisionManager)
{
    m_lastCollisionManager = collisionManager;
}

void PlayerMovement::ApplyJumpImpulse(float impulse)
{
    if (!m_physics.IsGrounded())
        return;
    Vector3 vel = m_physics.GetVelocity();
    vel.y = impulse;
    m_physics.SetVelocity(vel);
    m_physics.SetGroundLevel(false);
    m_player->GetPhysics().SetJumpState(true);
}

void PlayerMovement::ApplyGravity(float deltaTime)
{
    Vector3 vel = m_physics.GetVelocity();
    if (!m_physics.IsGrounded())
    {
        vel.y -= m_physics.GetGravity() * deltaTime;
        vel.y = std::max(vel.y, -50.0f); // обмеження падіння
        m_physics.SetVelocity(vel);
    }
    else if (vel.y < 0.0f)
    {
        vel.y = 0.0f;
        m_physics.SetVelocity(vel);
    }
}

Vector3 PlayerMovement::StepMovement(const CollisionManager &collisionManager)
{
    float dt = GetFrameTime();
    Vector3 vel = m_physics.GetVelocity();
    Vector3 targetPos = Vector3Add(m_position, Vector3Scale(vel, dt));

    // Вертикальна колізія
    SetPosition({targetPos.x, targetPos.y, targetPos.z});
    m_player->UpdatePlayerBox();
    m_player->UpdatePlayerCollision();

    Vector3 response;
    if (collisionManager.CheckCollision(m_player->GetCollision(), response))
    {
        targetPos = Vector3Add(m_position, response);
        if (response.y > 0.0f)
            vel.y = 0.0f; // стоїмо на землі
        if (response.y < 0.0f)
            vel.y = 0.0f; // удар об стелю
        SetPosition(targetPos);
        m_physics.SetVelocity(vel);
        m_physics.SetGroundLevel(response.y > 0.0f);
    }

    // Горизонтальна колізія
    targetPos.x = m_position.x + vel.x * dt;
    targetPos.z = m_position.z + vel.z * dt;
    SetPosition(targetPos);
    m_player->UpdatePlayerBox();
    m_player->UpdatePlayerCollision();

    if (collisionManager.CheckCollision(m_player->GetCollision(), response))
    {
        response.y = 0.0f; // ігнор вертикалі
        SetPosition(Vector3Add(m_position, response));
        m_player->UpdatePlayerBox();
    }

    return m_position;
}

void PlayerMovement::HandleCollisionVelocity(const Vector3 &responseMtv)
{
    Vector3 vel = m_physics.GetVelocity();

    if (responseMtv.y > 0.001f && vel.y <= 0.0f)
    {
        vel.y = 0.0f;
        m_physics.SetGroundLevel(true);
    }
    else
    {
        Vector3 normal = responseMtv;
        float len = Vector3Length(normal);
        if (len > 1e-6f)
            normal = Vector3Scale(normal, 1.0f / len);
        float vn = Vector3DotProduct(vel, normal);
        vel = Vector3Subtract(vel, Vector3Scale(normal, vn));
    }

    m_physics.SetVelocity(vel);
}

void PlayerMovement::SnapToGround(const CollisionManager &collisionManager)
{
    Vector3 checkPos = m_position;
    checkPos.y -= 0.5f; // невелике опускання
    SetPosition(checkPos);
    m_player->UpdatePlayerBox();

    Vector3 response;
    if (collisionManager.CheckCollision(m_player->GetCollision(), response) && response.y > 0.0f)
    {
        m_position.y += response.y;
        SetPosition(m_position);
        m_physics.SetGroundLevel(true);
        Vector3 vel = m_physics.GetVelocity();
        vel.y = 0.0f;
        m_physics.SetVelocity(vel);
    }
    else
    {
        m_physics.SetGroundLevel(false);
    }
}

bool PlayerMovement::ExtractFromCollider()
{
    if (!m_lastCollisionManager)
        return false;

    if (!m_physics.IsGrounded() || fabsf(m_physics.GetVelocity().y) > 0.1f)
        return false;

    m_player->UpdatePlayerBox();
    Vector3 response;
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), response))
        return false;

    SetPosition(Vector3Add(m_position, {0, 0.5f, 0}));
    m_player->UpdatePlayerBox();

    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), response))
        return true;

    SetPosition(Player::DEFAULT_SPAWN_POSITION);
    m_physics.SetVelocity({0, 0, 0});
    m_physics.SetGroundLevel(false);
    return true;
}

#endif