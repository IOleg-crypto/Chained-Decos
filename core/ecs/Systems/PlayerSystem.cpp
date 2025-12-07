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
    auto &render = RenderManager::Get();
    Camera &camera = render.GetCamera();

    Vector3 moveDir = {0, 0, 0};

    // Input relative to camera
    // We get the camera's forward and right vectors (projected on XZ plane)
    Vector3 forward = Vector3Subtract(camera.target, camera.position);
    forward.y = 0; // Flatten on XZ
    forward = Vector3Normalize(forward);

    // Reverting Cross Product based on user feedback.
    // Ideally Up x Forward = Right. But user reports inversion.
    // Switching to Forward x Up (which is mathematically "Left" in RHS).
    // This implies D (Right Key) will move "Left" in World Space,
    // which effectively aligns with "Screen Right" if the camera is
    // oriented opposite to expectations or if X axis is inverted.
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

        // Update player rotation to face movement direction
        float targetAngle = atan2f(moveDir.x, moveDir.z) * RAD2DEG;

        // Basic interpolation for rotation could go here, but for now snap or simple
        // transform.rotation.y = targetAngle; // Optional: Rotate player model to face move dir
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

    // Update Player Yaw (Model Rotation Y)
    transform.rotation.y -= mouseDelta.x * 0.1f;

    // Update Camera Pitch (Separate from Model Rotation)
    player.cameraPitch -= mouseDelta.y * 0.1f;

    // Clamp vertical rotation (pitch)
    if (player.cameraPitch > 89.0f)
        player.cameraPitch = 89.0f;
    if (player.cameraPitch < -89.0f)
        player.cameraPitch = -89.0f;

    // Ensure model does not pitch/roll
    transform.rotation.x = 0.0f;
    transform.rotation.z = 0.0f;

    // Third Person Camera Math
    float yawRad = transform.rotation.y * DEG2RAD;
    float pitchRad = player.cameraPitch * DEG2RAD;

    // Distance from player
    float distance = 5.0f;

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

void Update(float deltaTime)
{
    auto view = REGISTRY.view<TransformComponent, VelocityComponent, PlayerComponent>();

    for (auto [entity, transform, velocity, player] : view.each())
    {
        HandleMovement(transform, velocity, player, deltaTime);
        HandleJump(velocity, player);
        HandleCamera(transform, player);
        HandleAudio(player, velocity);
    }
}

} // namespace PlayerSystem
