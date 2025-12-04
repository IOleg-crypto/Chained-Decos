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

namespace PlayerSystem
{

static void HandleMovement(TransformComponent &transform, VelocityComponent &velocity,
                           PlayerComponent &player, float dt)
{
    auto &input = InputManager::Get();

    Vector3 moveDir = {0, 0, 0};

    if (input.IsKeyDown(KEY_W))
        moveDir.z += 1.0f;
    if (input.IsKeyDown(KEY_S))
        moveDir.z -= 1.0f;
    if (input.IsKeyDown(KEY_A))
        moveDir.x -= 1.0f;
    if (input.IsKeyDown(KEY_D))
        moveDir.x += 1.0f;

    // Нормалізувати
    float length = sqrtf(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
    if (length > 0.0f)
    {
        moveDir.x /= length;
        moveDir.z /= length;
    }

    // Застосувати швидкість
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

            // Звук стрибка
            AudioManager::Get().PlaySoundEffect("jump.wav");
        }
    }

    // Скинути стрибки при приземленні
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

    // Оновити камеру
    Camera &camera = render.GetCamera();
    camera.position = transform.position;
    camera.position.y += 1.8f; // Висота очей

    // Обертання камери (спрощено)
    transform.rotation.y -= mouseDelta.x * 0.1f;
    transform.rotation.x -= mouseDelta.y * 0.1f;

    // Clamp vertical rotation
    if (transform.rotation.x > 89.0f)
        transform.rotation.x = 89.0f;
    if (transform.rotation.x < -89.0f)
        transform.rotation.x = -89.0f;
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
