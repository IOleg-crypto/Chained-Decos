#include "engine/physics.h"
#include <gtest/gtest.h>

using namespace CH;

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

TEST(PhysicsTest, ColliderOffsetAndSize)
{
    // This would require a scene and entities to test the Physics::Update logic
    // But for now, we focus on the core AABB check.
}
