#include "player_system.h"
#include "core/audio/audio.h"
#include "core/input/input.h"
#include "core/log.h"
#include "core/renderer/renderer.h"
#include "scene/ecs/components/player_component.h"
#include "scene/ecs/components/transform_component.h"
#include "scene/ecs/components/velocity_component.h"
#include <cmath>
#include <raylib.h>
#include <raymath.h>


namespace CHEngine
{
void PlayerSystem::Update(entt::registry &registry, float deltaTime)
{
    auto view = registry.view<TransformComponent, VelocityComponent, PlayerComponent>();

    for (auto [entity, transform, velocity, player] : view.each())
    {
        // 1. Update Stats
        player.runTimer += deltaTime;
        if (transform.position.y > player.maxHeight)
            player.maxHeight = transform.position.y;

        // 2. Handle Camera Input
        Vector2 mouseDelta = Input::GetMouseDelta();
        player.cameraDistance -= Input::GetMouseWheelMove() * 1.5f;
        player.cameraDistance = Clamp(player.cameraDistance, 2.0f, 20.0f);
        player.cameraYaw -= mouseDelta.x * player.mouseSensitivity;
        player.cameraPitch =
            Clamp(player.cameraPitch - mouseDelta.y * player.mouseSensitivity, -85.0f, 85.0f);

        // 3. Handle Movement Direction
        Vector3 moveDir = {0, 0, 0};
        float yawRad = player.cameraYaw * DEG2RAD;
        Vector3 forward = Vector3Normalize({-sinf(yawRad), 0.0f, -cosf(yawRad)});
        Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));

        if (Input::IsKeyDown(KEY_W))
            moveDir = Vector3Add(moveDir, forward);
        if (Input::IsKeyDown(KEY_S))
            moveDir = Vector3Subtract(moveDir, forward);
        if (Input::IsKeyDown(KEY_D))
            moveDir = Vector3Add(moveDir, right);
        if (Input::IsKeyDown(KEY_A))
            moveDir = Vector3Subtract(moveDir, right);

        float moveLen = Vector3Length(moveDir);
        if (moveLen > 0.0f)
        {
            moveDir = Vector3Scale(moveDir, 1.0f / moveLen);
            float targetAngle = atan2f(moveDir.x, moveDir.z);
            float rotationSpeed = 10.0f;
            transform.rotation.y = transform.rotation.y +
                                   (targetAngle - transform.rotation.y) * rotationSpeed * deltaTime;
        }

        // 4. Update Velocity
        float targetSpeed = player.moveSpeed;
        if (Input::IsKeyDown(KEY_LEFT_SHIFT) && player.isGrounded)
            targetSpeed *= 1.8f;

        if (player.isGrounded)
        {
            velocity.velocity.x = moveDir.x * targetSpeed;
            velocity.velocity.z = moveDir.z * targetSpeed;
        }
        else
        {
            float airControl = 0.3f;
            velocity.velocity.x += moveDir.x * targetSpeed * airControl * deltaTime;
            velocity.velocity.z += moveDir.z * targetSpeed * airControl * deltaTime;
            float currentSpeed = sqrtf(velocity.velocity.x * velocity.velocity.x +
                                       velocity.velocity.z * velocity.velocity.z);
            if (currentSpeed > targetSpeed)
            {
                velocity.velocity.x *= targetSpeed / currentSpeed;
                velocity.velocity.z *= targetSpeed / currentSpeed;
            }
        }

        // 5. Jump
        if (Input::IsKeyPressed(KEY_SPACE) && player.isGrounded)
        {
            velocity.velocity.y = player.jumpForce;
            player.isGrounded = false;
        }

        // 6. Audio Update (Falling Sound)
        const float fallThresh = -5.0f;
        if (velocity.velocity.y < fallThresh && !player.isFallingSoundPlaying)
        {
            Audio::PlayLoopingSoundEffect("player_fall", 1.0f);
            player.isFallingSoundPlaying = true;
        }
        else if ((velocity.velocity.y >= fallThresh || player.isGrounded) &&
                 player.isFallingSoundPlaying)
        {
            Audio::StopLoopingSoundEffect("player_fall");
            player.isFallingSoundPlaying = false;
        }

        // 7. Update Camera (Hazel-style)
        Camera3D camera = Renderer::GetCamera();
        float yawRadLocal = player.cameraYaw * DEG2RAD;
        float pitchRadLocal = player.cameraPitch * DEG2RAD;

        Vector3 camOffset = {player.cameraDistance * cosf(pitchRadLocal) * sinf(yawRadLocal),
                             player.cameraDistance * sinf(pitchRadLocal),
                             player.cameraDistance * cosf(pitchRadLocal) * cosf(yawRadLocal)};

        camera.target = Vector3Add(transform.position, {0, 1.5f, 0});
        camera.position = Vector3Add(camera.target, camOffset);
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovy = 60.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        Renderer::SetCamera(camera);
    }

    // 8. Handle Respawn (F)
    if (::IsKeyPressed(KEY_F))
    {
        auto respawnView = registry.view<TransformComponent, VelocityComponent, PlayerComponent>();
        for (auto &&[entity, transform, velocity, player] : respawnView.each())
        {
            transform.position = player.spawnPosition;
            velocity.velocity = {0, 0, 0};
            player.isGrounded = false;
            player.runTimer = 0;
            player.maxHeight = 0;
            player.cameraDistance = 10.0f;
            player.cameraPitch = 25.0f;
            player.cameraYaw = 0.0f;
            if (player.isFallingSoundPlaying)
            {
                Audio::StopLoopingSoundEffect("player_fall");
                player.isFallingSoundPlaying = false;
            }
            CD_INFO("[PlayerSystem] Player respawned at (%.2f, %.2f, %.2f)", player.spawnPosition.x,
                    player.spawnPosition.y, player.spawnPosition.z);
        }
    }
}
} // namespace CHEngine
