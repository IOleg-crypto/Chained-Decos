#include <Player/Player.h>
#include <Player/PlayerInput.h>

PlayerInput::PlayerInput(Player *player) : m_player(player) {}

void PlayerInput::ProcessInput()
{
    float deltaTime = GetFrameTime();

    Vector3 inputDirection = GetInputDirection();

    auto [forward, right] = GetCameraVectors();
    forward.y = 0.0f;
    right.y = 0.0f;
    forward = Vector3Normalize(forward);
    right = Vector3Normalize(right);

    Vector3 worldMoveDir =
        Vector3Add(Vector3Scale(right, inputDirection.x), Vector3Scale(forward, inputDirection.z));
    worldMoveDir.y = 0.0f;

    if (Vector3Length(worldMoveDir) > 0.001f)
        worldMoveDir = Vector3Normalize(worldMoveDir);

    Vector3 velocity = m_player->GetPhysics().GetVelocity();

    if (Vector3Length(worldMoveDir) > 0.001f)
    {
        if (m_player->GetPhysics().IsGrounded())
        {
            velocity.x = worldMoveDir.x * m_player->GetSpeed();
            velocity.z = worldMoveDir.z * m_player->GetSpeed();
        }
        else
        {

            const float AIR_CONTROL = 0.1f;
            velocity.x += worldMoveDir.x * m_player->GetSpeed() * AIR_CONTROL * deltaTime;
            velocity.z += worldMoveDir.z * m_player->GetSpeed() * AIR_CONTROL * deltaTime;
        }
    }
    else if (m_player->GetPhysics().IsGrounded())
    {

        const float FRICTION = 0.85f;
        velocity.x *= FRICTION;
        velocity.z *= FRICTION;

        const float STOP_THRESHOLD = 0.1f;
        if (fabsf(velocity.x) < STOP_THRESHOLD)
            velocity.x = 0.0f;
        if (fabsf(velocity.z) < STOP_THRESHOLD)
            velocity.z = 0.0f;
    }

    m_player->GetPhysics().SetVelocity(velocity);
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
    forward.y = 0.0f;
    forward = Vector3Normalize(forward);

    // Right: перпендикулярно forward та up
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));

    return {forward, right};
}