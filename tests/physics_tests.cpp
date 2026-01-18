#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include <gtest/gtest.h>

using namespace CHEngine;

TEST(PhysicsTest, AABBIntersection)
{
    Vector3 minA = {0.0f, 0.0f, 0.0f};
    Vector3 maxA = {1.0f, 1.0f, 1.0f};

    Vector3 minB = {0.5f, 0.5f, 0.5f};
    Vector3 maxB = {1.5f, 1.5f, 1.5f};

    // Obvious overlap
    EXPECT_TRUE(Physics::CheckAABB(minA, maxA, minB, maxB));
    EXPECT_TRUE(Physics::CheckAABB(minB, maxB, minA, maxA));

    // Touching on edge (should be true based on <= and >=)
    Vector3 minC = {1.0f, 0.0f, 0.0f};
    Vector3 maxC = {2.0f, 1.0f, 1.0f};
    EXPECT_TRUE(Physics::CheckAABB(minA, maxA, minC, maxC));

    // No overlap
    Vector3 minD = {1.1f, 0.0f, 0.0f};
    Vector3 maxD = {2.1f, 1.0f, 1.0f};
    EXPECT_FALSE(Physics::CheckAABB(minA, maxA, minD, maxD));
}

TEST(PhysicsTest, Raycast)
{
    auto scene = CreateRef<Scene>();
    auto entity = scene->CreateEntity("Test Entity");
    auto &transform = entity.GetComponent<TransformComponent>();
    transform.Translation = {0.0f, 0.0f, 5.0f};

    auto &collider = entity.AddComponent<ColliderComponent>();
    collider.Size = {1.0f, 1.0f, 1.0f};
    collider.Offset = {-0.5f, -0.5f, -0.5f}; // Center it

    // Ray from origin looking forward
    Ray ray;
    ray.position = {0.0f, 0.0f, 0.0f};
    ray.direction = {0.0f, 0.0f, 1.0f};

    RaycastResult result = Physics::Raycast(scene.get(), ray);
    EXPECT_TRUE(result.Hit);
    EXPECT_NEAR(result.Distance, 4.5f, 0.001f);
    EXPECT_EQ(result.Entity, (entt::entity)entity);

    // Ray looking away
    ray.direction = {0.0f, 0.0f, -1.0f};
    result = Physics::Raycast(scene.get(), ray);
    EXPECT_FALSE(result.Hit);
}
