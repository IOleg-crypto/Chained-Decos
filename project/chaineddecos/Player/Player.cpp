#include "Player.h"
#include "input/Core/InputManager.h"

Player::Player()
{
    // Initialize defaults
    m_bounds.Size = {1.0f, 2.0f, 1.0f};
    m_bounds.Center = {0.0f, 5.0f, 0.0f}; // Start in air
}

void Player::Update(float deltaTime)
{
    HandleInput();
    UpdatePhysics(deltaTime);
}

void Player::SetPosition(const Vector3 &pos)
{
    m_bounds.Center = pos;
}

void Player::Jump()
{
    m_physics.Jump();
}

void Player::Move(const Vector3 &direction)
{
    // Simple movement
    Vector3 move = Vector3Scale(direction, m_speed);
    m_physics.Velocity.x = move.x;
    m_physics.Velocity.z = move.z;
}

void Player::HandleInput()
{
    // Get input from Servers
    Vector3 moveDir = {0, 0, 0};

    if (Servers::InputManager::IsDown(KEY_W))
        moveDir.z -= 1.0f;
    if (Servers::InputManager::IsDown(KEY_S))
        moveDir.z += 1.0f;
    if (Servers::InputManager::IsDown(KEY_A))
        moveDir.x -= 1.0f;
    if (Servers::InputManager::IsDown(KEY_D))
        moveDir.x += 1.0f;

    if (Servers::InputManager::IsPressed(KEY_SPACE))
    {
        Jump();
    }

    // Normalize if moving diagonally
    if (Vector3Length(moveDir) > 0)
    {
        moveDir = Vector3Normalize(moveDir);
        Move(moveDir);

        // Update rotation to face movement
        float targetAngle = atan2f(moveDir.x, moveDir.z) * RAD2DEG;
        SetRotationY(targetAngle);
    }
    else
    {
        // Stop horizontal movement
        m_physics.Velocity.x = 0;
        m_physics.Velocity.z = 0;
    }
}

void Player::UpdatePhysics(float dt)
{
    m_physics.ApplyGravity(dt);

    // Apply movement
    Vector3 move = m_physics.GetMovement(dt);
    m_bounds.Center = Vector3Add(m_bounds.Center, move);

    // Simple ground collision (y = 0)
    if (m_bounds.Center.y <= 1.0f)
    { // 1.0 is half height
        m_bounds.Center.y = 1.0f;
        m_physics.Velocity.y = 0;
        m_physics.IsGrounded = true;
    }
    else
    {
        m_physics.IsGrounded = false;
    }
}
Servers::Physics &Player::GetPhysics()
{
    return m_physics;
}
const Servers::Bounds &Player::GetBounds() const
{
    return m_bounds;
}
Vector3 Player::GetPosition() const
{
    return m_bounds.Center;
}
float Player::GetRotationY() const
{
    return m_rotationY;
}
void Player::SetRotationY(float rotation)
{
    m_rotationY = rotation;
}
