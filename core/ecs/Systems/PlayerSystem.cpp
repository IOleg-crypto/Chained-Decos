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

            // Play jump sound (using existing file as placeholder)
            AudioManager::Get().PlaySoundEffect("player_fall");
        }
    }

    if (player.isGrounded)
    {
        player.jumpsRemaining = player.canDoubleJump ? 1 : 0;
    }
}

static void HandleCamera(TransformComponent &transform)
{
    auto &input = InputManager::Get();
    auto &render = RenderManager::Get();

    Vector2 mouseDelta = input.GetMouseDelta();

    // Update Camera Angles (stored in transform rotation for now, or separate component)
    // Using transform.rotation to store camera look direction is common in simple setups,
    // but for TPV, player model rotation vs camera rotation are different.
    // However, let's stick to using transform.rotation as the "View Angle" source for consistency
    // with previous code.

    transform.rotation.y -= mouseDelta.x * 0.1f;
    transform.rotation.x -= mouseDelta.y * 0.1f;

    // Clamp vertical rotation
    if (transform.rotation.x > 89.0f)
        transform.rotation.x = 89.0f;
    if (transform.rotation.x < -89.0f)
        transform.rotation.x = -89.0f;

    // Third Person Camera Math
    float yawRad = transform.rotation.y * DEG2RAD;
    float pitchRad = transform.rotation.x * DEG2RAD;

    // Distance from player
    float distance = 5.0f;

    // Calculate offset based on yaw/pitch
    Vector3 offset;
    offset.x = sinf(yawRad) * cosf(pitchRad) * distance;
    offset.y = sinf(pitchRad) * distance;
    offset.z = cosf(yawRad) * cosf(pitchRad) * distance;

    // Camera Position = PlayerPos - Offset (Behind player)
    // We want the camera to look AT the player.
    // Note: Standard TPV usually has camera rotate AROUND player.
    // Pitch up = Camera goes down? transform.rotation.x is usually "Look Up/Down".
    // If Pitch is Look Up (+), y increases?
    // Let's use standard spherical coordinates.

    // Recalculate for "Orbit" style:
    // x = r * sin(theta) * cos(phi)
    // y = r * sin(phi)
    // z = r * cos(theta) * cos(phi)

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

void Update(float deltaTime)
{
    auto view = REGISTRY.view<TransformComponent, VelocityComponent, PlayerComponent>();

    for (auto [entity, transform, velocity, player] : view.each())
    {
        HandleMovement(transform, velocity, player, deltaTime);
        HandleJump(velocity, player);
        HandleCamera(transform);
    }
}

} // namespace PlayerSystem
