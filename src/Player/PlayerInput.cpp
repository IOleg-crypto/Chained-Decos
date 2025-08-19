#include <Player/PlayerInput.h>
#include <Player/Player.h>

PlayerInput::PlayerInput(Player* player) : m_player(player)
{
}

void PlayerInput::ProcessInput()
{
    float deltaTime = GetFrameTime();
    m_player->GetPhysics().Update(deltaTime);

    Vector3 inputDirection = GetInputDirection();
    if (Vector3Length(inputDirection) < 0.001f) return;

    inputDirection = Vector3Normalize(inputDirection);
    auto [forward, right] = GetCameraVectors();

    forward.y = 0.0f;
    right.y = 0.0f;
    forward = Vector3Normalize(forward);
    right = Vector3Normalize(right);

    Vector3 worldMoveDir = {right.x * inputDirection.x + forward.x * inputDirection.z, 0.0f,
                            right.z * inputDirection.x + forward.z * inputDirection.z};

    if (Vector3Length(worldMoveDir) > 0.001f)
        worldMoveDir = Vector3Normalize(worldMoveDir);

    if (m_player->GetPhysics().IsGrounded())
        m_player->ApplyGroundedMovement(worldMoveDir, deltaTime);
    else
        m_player->ApplyAirborneMovement(worldMoveDir, deltaTime);
}

void PlayerInput::HandleJumpInput()
{
    if (IsKeyDown(KEY_SPACE) && m_player->GetPhysics().IsGrounded())
    {
        m_player->ApplyJumpImpulse(m_player->GetPhysics().GetJumpStrength() * 3.0f);
    }
}

void PlayerInput::HandleEmergencyReset()
{
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
        inputDir.z -= 1.0f;
    if (IsKeyDown(KEY_S))
        inputDir.z += 1.0f;
    if (IsKeyDown(KEY_A))
        inputDir.x -= 1.0f;
    if (IsKeyDown(KEY_D))
        inputDir.x += 1.0f;

    // Update speed based on sprint key
    float walkSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? 15.0f : 8.1f;
    m_player->SetSpeed(walkSpeed);

    return inputDir;
}

std::pair<Vector3, Vector3> PlayerInput::GetCameraVectors() const
{
    const Camera &camera = m_player->GetCameraController()->GetCamera();

    Vector3 forward = Vector3Subtract(camera.position, camera.target);
    forward.y = 0;
    forward = Vector3Normalize(forward);

    Vector3 right = Vector3Normalize(Vector3CrossProduct({0, 1, 0}, forward));

    return {forward, right};
}