#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>

#include "Engine/Collision/CollisionManager.h"
#include "Engine/Collision/CollisionSystem.h"
#include "Engine/Math/Vector3.h"

class CollisionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        collisionManager = std::make_shared<CollisionManager>();
    }

    void TearDown() override {
        collisionManager.reset();
    }

    std::shared_ptr<CollisionManager> collisionManager;
};

TEST_F(CollisionManagerTest, ConstructorInitializesEmpty) {
    EXPECT_EQ(collisionManager->GetColliders().size(), 0);
}

TEST_F(CollisionManagerTest, AddColliderWorks) {
    Collision collider(Vector3{0, 0, 0}, Vector3{1, 1, 1});

    collisionManager->AddCollider(std::move(collider));

    EXPECT_EQ(collisionManager->GetColliders().size(), 1);
}

TEST_F(CollisionManagerTest, ClearCollidersWorks) {
    Collision collider(Vector3{0, 0, 0}, Vector3{1, 1, 1});
    collisionManager->AddCollider(std::move(collider));
    EXPECT_EQ(collisionManager->GetColliders().size(), 1);

    collisionManager->ClearColliders();
    EXPECT_EQ(collisionManager->GetColliders().size(), 0);
}

TEST_F(CollisionManagerTest, CheckCollisionDetectsOverlappingBoxes) {
    Collision collider1(Vector3{0, 0, 0}, Vector3{1, 1, 1});
    Collision collider2(Vector3{0.5f, 0.5f, 0.5f}, Vector3{1, 1, 1});

    collisionManager->AddCollider(std::move(collider1));

    bool collision = collisionManager->CheckCollision(collider2);
    EXPECT_TRUE(collision);
}

TEST_F(CollisionManagerTest, CheckCollisionDetectsNonOverlappingBoxes) {
    Collision collider1(Vector3{0, 0, 0}, Vector3{1, 1, 1});
    Collision collider2(Vector3{3, 3, 3}, Vector3{1, 1, 1});

    collisionManager->AddCollider(std::move(collider1));

    bool collision = collisionManager->CheckCollision(collider2);
    EXPECT_FALSE(collision);
}

TEST_F(CollisionManagerTest, CheckCollisionWithResponse) {
    Collision collider1(Vector3{0, 0, 0}, Vector3{1, 1, 1});
    Collision collider2(Vector3{0.5f, 0.5f, 0.5f}, Vector3{1, 1, 1});

    collisionManager->AddCollider(std::move(collider1));

    Vector3 response;
    bool collision = collisionManager->CheckCollision(collider2, response);
    EXPECT_TRUE(collision);
}

TEST_F(CollisionManagerTest, RaycastDownWorks) {
    Collision collider(Vector3{0, -1, 0}, Vector3{2, 1, 2});
    collisionManager->AddCollider(std::move(collider));

    float hitDistance;
    Vector3 hitPoint, hitNormal;
    bool hit = collisionManager->RaycastDown(Vector3{0, 1, 0}, 10.0f, hitDistance, hitPoint, hitNormal);
    EXPECT_TRUE(hit);
    EXPECT_NEAR(hitDistance, 2.0f, 0.1f); // From y=1 to y=-1 (surface at y=-1, half-height 1)
}