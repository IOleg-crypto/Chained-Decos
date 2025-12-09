#include "PlayerSystem.h"
#include "components/audio/Core/AudioManager.h"
#include "components/input/Core/InputManager.h"
#include "components/rendering/Core/RenderManager.h"
#include "core/ecs/Components/PlayerComponent.h"
#include "core/ecs/Components/TransformComponent.h"
#include "core/ecs/Components/VelocityComponent.h"
#include "core/ecs/ECSRegistry.h"
#include <cmath>
#include <raylib.h>
#include <raymath.h>

namespace PlayerSystem
{

static void HandleMovement(TransformComponent &transform, VelocityComponent &velocity,
                           PlayerComponent &player, float dt)
{
    auto &input = InputManager::Get();
    // auto &render = RenderManager::Get();
    // Camera &camera = render.GetCamera(); // Not needed for direction calc if we use stored Yaw

    Vector3 moveDir = {0, 0, 0};

    // Calculate Forward/Right vectors based on Camera Yaw
    // Forward is (sin(yaw), 0, cos(yaw))
    float yawRad = player.cameraYaw * DEG2RAD;
    Vector3 forward;
    forward.x = sinf(yawRad);
    forward.y = 0.0f;
    forward.z = cosf(yawRad);
    forward = Vector3Normalize(forward);

    // Right vector (Forward x Up)
    Vector3 right = Vector3CrossProduct(forward, {0, 1, 0});
    right = Vector3Normalize(right);

    if (input.IsKeyDown(KEY_W))
        moveDir = Vector3Add(moveDir, forward);
    if (input.IsKeyDown(KEY_S))
        moveDir = Vector3Subtract(moveDir, forward);
    if (input.IsKeyDown(KEY_D))
        moveDir = Vector3Add(moveDir, right);
    if (input.IsKeyDown(KEY_A))
        moveDir = Vector3Subtract(moveDir, right);

    // Normalize
    float length = Vector3Length(moveDir);
    if (length > 0.0f)
    {
        moveDir = Vector3Scale(moveDir, 1.0f / length);

        // Update player rotation to face movement direction ONLY when moving
        float targetAngle = atan2f(moveDir.x, moveDir.z) * RAD2DEG;

        // Simple interpolation or direct set for responsiveness
        // For now, direct set to ensure it works immediately
        transform.rotation.y = targetAngle;
    }

    // Apply velocity
    velocity.velocity.x = moveDir.x * player.moveSpeed;
    velocity.velocity.z = moveDir.z * player.moveSpeed;
}

static void HandleJump(VelocityComponent &velocity, PlayerComponent &player)
{
    auto &input = InputManager::Get();

    if (input.IsKeyPressed(KEY_SPACE))
    {
        if (player.isGrounded || player.jumpsRemaining > 0)
        {
            velocity.velocity.y = player.jumpForce;

            if (!player.isGrounded)
            {
                player.jumpsRemaining--;
            }

            // Jump sound removed (was using incorrect player_fall)
            // AudioManager::Get().PlaySoundEffect("jump"); // TODO: Add jump sound
        }
    }

    if (player.isGrounded)
    {
        player.jumpsRemaining = player.canDoubleJump ? 1 : 0;
    }
}

static void HandleCamera(TransformComponent &transform, PlayerComponent &player)
{
    auto &input = InputManager::Get();
    auto &render = RenderManager::Get();

    Vector2 mouseDelta = input.GetMouseDelta();

    // Update Camera Yaw (Independent of Player Model)
    player.cameraYaw -= mouseDelta.x * 0.1f;

    // Update Camera Pitch
    player.cameraPitch -= mouseDelta.y * 0.1f;

    // Clamp vertical rotation (pitch)
    if (player.cameraPitch > 89.0f)
        player.cameraPitch = 89.0f;
    if (player.cameraPitch < -89.0f)
        player.cameraPitch = -89.0f;

    // Ensure model does not pitch/roll (safety)
    transform.rotation.x = 0.0f;
    transform.rotation.z = 0.0f;

    // Third Person Camera Math using cameraYaw
    float yawRad = player.cameraYaw * DEG2RAD;
    float pitchRad = player.cameraPitch * DEG2RAD;

    // Zoom Logic
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0.0f)
    {
        player.cameraDistance -= wheelMove * 1.5f; // Adjust sensitivity as needed
        if (player.cameraDistance < 2.0f)
            player.cameraDistance = 2.0f;
        if (player.cameraDistance > 15.0f)
            player.cameraDistance = 15.0f;
    }

    // Distance from player
    float distance = player.cameraDistance;

    // Calculate offset based on yaw/pitch
    Vector3 offset;
    offset.x = sinf(yawRad) * cosf(pitchRad) * distance;
    offset.y = sinf(pitchRad) * distance;
    offset.z = cosf(yawRad) * cosf(pitchRad) * distance;

    // Position camera BEHIND the focus point
    // We use the REVERSE of the view vector
    Vector3 forward;
    forward.x = sinf(yawRad) * cosf(pitchRad);
    forward.y = sinf(pitchRad);
    forward.z = cosf(yawRad) * cosf(pitchRad);

    Vector3 viewDir = Vector3Normalize(forward);
    Vector3 cameraOffset = Vector3Scale(viewDir, -distance); // Behind

    Camera &camera = render.GetCamera();

    // Focus point is player + render offset (e.g. head/center)
    // IMPORTANT: transform.position is physics position. Visual is offset by -1.0.
    // If we want to look at the head (which is ~1.8m tall), and pivot is at feet (Visual-wise),
    // Physics position is Center (0.9?).
    // GameApp sets Visual Offset -1.0. This means Model Feet are at Phys-1.0.
    // Phys Center is likely +0.4 above ground?
    // Let's stick to transform.position + offset for now.
    Vector3 focusPoint = transform.position;
    focusPoint.y += 1.5f; // Look at center/head

    camera.position = Vector3Add(focusPoint, cameraOffset);
    camera.target = focusPoint;
}

static void HandleAudio(PlayerComponent &player, const VelocityComponent &velocity)
{
    // Threshold for falling sound
    const float FALL_THRESHOLD = -8.0f; // Falling speed threshold

    bool isFallingFast = (velocity.velocity.y < FALL_THRESHOLD) && !player.isGrounded;

    if (isFallingFast)
    {
        if (!player.isFallingSoundPlaying)
        {
            AudioManager::Get().PlayLoopingSoundEffect("player_fall", 1.0f);
            player.isFallingSoundPlaying = true;
        }
    }
    else
    {
        if (player.isFallingSoundPlaying)
        {
            AudioManager::Get().StopLoopingSoundEffect("player_fall");
            player.isFallingSoundPlaying = false;
        }
    }
}

static void HandleRespawn(TransformComponent &transform, VelocityComponent &velocity,
                          PlayerComponent &player)
{
    auto &input = InputManager::Get();
    if (input.IsKeyPressed(KEY_F))
    {
        // Reset position to default spawn
        transform.position = {0.0f, 2.0f, 0.0f};
        velocity.velocity = {0.0f, 0.0f, 0.0f};
        player.isGrounded = false;

        // Reset stats
        player.runTimer = 0.0f;
        player.maxHeight = 0.0f;

        // Stop any falling sound
        if (player.isFallingSoundPlaying)
        {
            AudioManager::Get().StopLoopingSoundEffect("player_fall");
            player.isFallingSoundPlaying = false;
        }
    }
}

void Update(float deltaTime)
{
    auto view = REGISTRY.view<TransformComponent, VelocityComponent, PlayerComponent>();

    for (auto [entity, transform, velocity, player] : view.each())
    {
        // Update Stats
        player.runTimer += deltaTime;
        if (transform.position.y > player.maxHeight)
        {
            player.maxHeight = transform.position.y;
        }

        HandleMovement(transform, velocity, player, deltaTime);
        HandleJump(velocity, player);
        HandleCamera(transform, player);
        HandleRespawn(transform, velocity, player);
        HandleAudio(player, velocity);
    }
}

} // namespace PlayerSystem
