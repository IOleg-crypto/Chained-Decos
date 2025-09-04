#include <Player.h>
#include <PlayerInput.h>

PlayerInput::PlayerInput(Player *player) : m_player(player) {}

void PlayerInput::ProcessInput()
{
    Vector3 inputDirection = GetInputDirection();
    float deltaTime = GetFrameTime();

    // Compute camera-aligned move direction
    Vector3 moveDir = {0};
    if (Vector3Length(inputDirection) >= 0.001f)
    {
        inputDirection = Vector3Normalize(inputDirection);
        auto [forward, right] = GetCameraVectors();

        forward.y = 0.0f;
        right.y = 0.0f;
        forward = Vector3Normalize(forward);
        right = Vector3Normalize(right);

        moveDir.x = forward.x * inputDirection.z + right.x * inputDirection.x;
        moveDir.z = forward.z * inputDirection.z + right.z * inputDirection.x;
        if (Vector3Length(moveDir) > 0.001f)
            moveDir = Vector3Normalize(moveDir);
    }

    // Convert desired move direction into horizontal velocity (units per second)
    float speed = m_player->GetSpeed();
    Vector3 desiredHorizontalVelocity = {moveDir.x * speed, 0.0f, moveDir.z * speed};

    // Apply to player's physics velocity preserving vertical component
    Vector3 v = m_player->GetPhysics().GetVelocity();
    v.x = desiredHorizontalVelocity.x;
    v.z = desiredHorizontalVelocity.z;
    m_player->GetPhysics().SetVelocity(v);

    // Smoothly rotate player towards movement direction when moving
    if (Vector3Length(desiredHorizontalVelocity) > 0.001f)
    {
        float currentRotY = m_player->GetRotationY();
        float targetRotY = atan2f(moveDir.x, moveDir.z) * RAD2DEG;
        float smoothEffect = 15.0f;
        float smoothRotY = currentRotY + (targetRotY - currentRotY) * smoothEffect * deltaTime;
        m_player->SetRotationY(smoothRotY);
    }
}

void PlayerInput::HandleJumpInput() const {
    if (IsKeyDown(KEY_SPACE) && m_player->GetPhysics().IsGrounded())
    {
        float jumpImpulse = m_player->GetPhysics().GetJumpStrength() * 3.0f;
        m_player->ApplyJumpImpulse(jumpImpulse);
    }
}

void PlayerInput::HandleEmergencyReset() const {
    if (IsKeyPressed(KEY_T))
    {
        m_player->SetPlayerPosition(Player::DEFAULT_SPAWN_POSITION);
        m_player->GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});
        m_player->GetPhysics().SetGroundLevel(false);
    }
}

Vector3 PlayerInput::GetInputDirection()
{
    Vector3 inputDir = {};

    if (IsKeyDown(KEY_W))
        inputDir.z += 1.0f;
    if (IsKeyDown(KEY_S))
        inputDir.z -= 1.0f;
    if (IsKeyDown(KEY_A))
        inputDir.x -= 1.0f;
    if (IsKeyDown(KEY_D))
        inputDir.x += 1.0f;

    // Update speed based on sprint key
    m_walkSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? 15.0f : 8.1f;
    m_player->SetSpeed(m_walkSpeed);

    return inputDir;
}

std::pair<Vector3, Vector3> PlayerInput::GetCameraVectors() const
{
    const Camera &camera = m_player->GetCameraController()->GetCamera();

    Vector3 forward = Vector3Subtract(camera.target, camera.position);
    forward.y = 0.0f;
    forward = Vector3Normalize(forward);

    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));

    return {forward, right};
}