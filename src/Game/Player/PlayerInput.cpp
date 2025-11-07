#include "PlayerInput.h"
#include "Player.h"
#include <imgui.h>

PlayerInput::PlayerInput(IPlayerMediator *player) : m_player(player), m_walkSpeed(8.1f) {}

void PlayerInput::ProcessInput()
{
    // Skip input processing if no window is available (for testing)
    if (!IsWindowReady())
    {
        return;
    }

    // Skip if ImGui wants keyboard capture (e.g., console open or text input active)
    // Only block input if text input is active OR navigation is both enabled and active
    // This ensures input works immediately after closing menu even if ImGui state hasn't fully reset
    ImGuiIO& io = ImGui::GetIO();
    bool navEnabled = (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
    if (io.WantCaptureKeyboard && (io.WantTextInput || (navEnabled && io.NavActive)))
    {
        return;
    }

    Vector3 inputDirection = GetInputDirection();

    // Use default delta time if no window is available (for testing)
    float deltaTime = IsWindowReady() ? GetFrameTime() : (1.0f / 60.0f);

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

void PlayerInput::HandleJumpInput() const
{
    // Skip key input if no window is available (for testing)
    if (!IsWindowReady())
    {
        return;
    }

    // Skip if ImGui wants keyboard capture (only for text input or active widgets)
    // Only block input if text input is active OR navigation is both enabled and active
    ImGuiIO& io = ImGui::GetIO();
    bool navEnabled = (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
    if (io.WantCaptureKeyboard && (io.WantTextInput || (navEnabled && io.NavActive)))
    {
        return;
    }

    if (IsKeyPressed(KEY_SPACE))
    {
        float jumpImpulse =
            m_player->GetPhysics().GetJumpStrength() * 1.2f; // Slightly stronger jump
        m_player->ApplyJumpImpulse(jumpImpulse);
        TraceLog(LOG_DEBUG, "PlayerInput::HandleJumpInput() - Jump key pressed, impulse: %.2f",
                 jumpImpulse);
    }
}

void PlayerInput::HandleEmergencyReset() const
{
    // Skip key input if no window is available (for testing)
    if (!IsWindowReady())
    {
        return;
    }

    // Skip if ImGui wants keyboard capture (only for text input or active widgets)
    // Only block input if text input is active OR navigation is both enabled and active
    ImGuiIO& io = ImGui::GetIO();
    bool navEnabled = (io.ConfigFlags & ImGuiConfigFlags_NavEnableKeyboard) != 0;
    if (io.WantCaptureKeyboard && (io.WantTextInput || (navEnabled && io.NavActive)))
    {
        return;
    }

    if (IsKeyPressed(KEY_T))
    {
        m_player->SetPlayerPosition(Player::DEFAULT_SPAWN_POSITION);
        m_player->GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});
        m_player->GetPhysics().SetGroundLevel(false);
    }
}

Vector3 PlayerInput::GetInputDirection()
{
    // Skip key input if no window is available (for testing)
    if (!IsWindowReady())
    {
        return {0.0f, 0.0f, 0.0f};
    }

    Vector3 inputDir = {};

    bool wDown = IsKeyDown(KEY_W);
    bool sDown = IsKeyDown(KEY_S);
    bool aDown = IsKeyDown(KEY_A);
    bool dDown = IsKeyDown(KEY_D);
    bool shiftDown = IsKeyDown(KEY_LEFT_SHIFT);

    if (wDown)
        inputDir.z += 1.0f;
    if (sDown)
        inputDir.z -= 1.0f;
    if (aDown)
        inputDir.x -= 1.0f;
    if (dDown)
        inputDir.x += 1.0f;

    bool isGrounded = m_player->GetPhysics().IsGrounded();

    // Update speed based on sprint key - only allow sprint when grounded
    if (isGrounded) // Only allow sprint when player is on ground
    {
        m_walkSpeed = shiftDown ? 15.0f : 8.1f;
    }
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