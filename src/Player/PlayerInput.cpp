#include <Player/Player.h>
#include <Player/PlayerInput.h>

PlayerInput::PlayerInput(Player *player) : m_player(player) {}

void PlayerInput::ProcessInput()
{
    Vector3 inputDirection = GetInputDirection();
    if (Vector3Length(inputDirection) < 0.001f)
        return;

    inputDirection = Vector3Normalize(inputDirection);
    auto [forward, right] = GetCameraVectors();

    forward.y = 0.0f;
    right.y = 0.0f;
    forward = Vector3Normalize(forward);
    right = Vector3Normalize(right);

    Vector3 moveDir = {};
    moveDir.x = forward.x * inputDirection.z + right.x * inputDirection.x;
    moveDir.z = forward.z * inputDirection.z + right.z * inputDirection.x;

    if (Vector3Length(moveDir) > 0.001f)
        moveDir = Vector3Normalize(moveDir);

    float deltaTime = GetFrameTime();
    float speed = m_player->GetSpeed();
    Vector3 velocity = Vector3Scale(moveDir, speed);

    m_player->Move(Vector3Scale(velocity, deltaTime));
}

void PlayerInput::HandleJumpInput()
{
    if (IsKeyDown(KEY_SPACE) && m_player->GetPhysics().IsGrounded())
    {
        float jumpImpulse = m_player->GetPhysics().GetJumpStrength() * 3.0f;
        m_player->ApplyJumpImpulse(jumpImpulse);
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
        inputDir.z += 1.0f;
    if (IsKeyDown(KEY_S))
        inputDir.z -= 1.0f;
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

    Vector3 forward = Vector3Subtract(camera.target, camera.position);
    forward.y = 0.0f;
    forward = Vector3Normalize(forward);

    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));

    return {forward, right};
}