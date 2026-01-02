#include "core/log.h"
#include "scene/core/Entity.h"
#include "scene/core/Scene.h"
#include "scene/ecs/components/render_component.h"
#include "scene/ecs/components/transform_component.h"

using namespace CHEngine;

/**
 * Simple test to validate Scene/Entity system
 *
 * This test creates a scene, adds entities, and verifies
 * component operations work correctly.
 */
int main()
{
    CD_CORE_INFO("=== Scene/Entity System Test ===");

    // Create a scene
    Scene scene("TestScene");
    CD_CORE_INFO("Created scene: %s", scene.GetName().c_str());

    // Create entities
    Entity player = scene.CreateEntity("Player");
    Entity enemy = scene.CreateEntity("Enemy");
    Entity camera = scene.CreateEntity("Camera");

    CD_CORE_INFO("Created 3 entities");

    // Add components to player
    auto &playerTransform = player.GetComponent<TransformComponent>();
    playerTransform.position = {0, 1, 0};

    auto &playerRender = player.AddComponent<RenderComponent>();
    playerRender.modelName = "player_low";
    playerRender.tint = WHITE;

    CD_CORE_INFO("Player position: (%.1f, %.1f, %.1f)", playerTransform.position.x,
                 playerTransform.position.y, playerTransform.position.z);

    // Add components to enemy
    auto &enemyTransform = enemy.GetComponent<TransformComponent>();
    enemyTransform.position = {5, 0, 5};

    auto &enemyRender = enemy.AddComponent<RenderComponent>();
    enemyRender.modelName = "enemy";
    enemyRender.tint = RED;

    // Verify component queries
    if (player.HasComponent<TransformComponent>())
    {
        CD_CORE_INFO("Player has TransformComponent");
    }

    if (player.HasComponent<RenderComponent>())
    {
        CD_CORE_INFO("Player has RenderComponent");
    }

    // Test entity iteration (EnTT v3.x API)
    int entityCount = 0;
    auto view = scene.GetRegistry().view<TransformComponent>();
    for (auto entity : view)
    {
        entityCount++;
    }

    CD_CORE_INFO("Total entities in scene: %d", entityCount);

    // Test entity destruction
    scene.DestroyEntity(enemy);
    CD_CORE_INFO("Destroyed enemy entity");

    entityCount = 0;
    auto view2 = scene.GetRegistry().view<TransformComponent>();
    for (auto entity : view2)
    {
        entityCount++;
    }

    CD_CORE_INFO("Entities after destruction: %d", entityCount);

    CD_CORE_INFO("=== Test Complete ===");
    return 0;
}
