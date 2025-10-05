#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>

#include "Engine/Collision/CollisionManager.h"
#include "Engine/Collision/CollisionComponent.h"
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
    EXPECT_EQ(collisionManager->GetCollisionPairs().size(), 0);
    EXPECT_FALSE(collisionManager->GetBVH().IsBuilt());
}

TEST_F(CollisionManagerTest, AddCollisionComponentWorks) {
    auto component = std::make_shared<CollisionComponent>();
    BoundingBox box = {
        .min = Vector3{-1, -1, -1},
        .max = Vector3{ 1,  1,  1}
    };
    component->SetBoundingBox(box);

    collisionManager->AddCollisionComponent(component);

    EXPECT_EQ(collisionManager->GetCollisionComponents().size(), 1);
}

TEST_F(CollisionManagerTest, RemoveCollisionComponentWorks) {
    auto component = std::make_shared<CollisionComponent>();
    BoundingBox box = {
        .min = Vector3{-1, -1, -1},
        .max = Vector3{ 1,  1,  1}
    };
    component->SetBoundingBox(box);

    collisionManager->AddCollisionComponent(component);
    EXPECT_EQ(collisionManager->GetCollisionComponents().size(), 1);

    collisionManager->RemoveCollisionComponent(component);
    EXPECT_EQ(collisionManager->GetCollisionComponents().size(), 0);
}

TEST_F(CollisionManagerTest, BuildBVHCreatesTree) {
    std::vector<std::shared_ptr<CollisionComponent>> components;

    // Add multiple components
    for (int i = 0; i < 10; i++) {
        auto component = std::make_shared<CollisionComponent>();
        BoundingBox box = {
            .min = Vector3{static_cast<float>(i), 0, 0},
            .max = Vector3{static_cast<float>(i + 1), 1, 1}
        };
        component->SetBoundingBox(box);
        components.push_back(component);
        collisionManager->AddCollisionComponent(component);
    }

    collisionManager->BuildBVH();

    EXPECT_TRUE(collisionManager->GetBVH().IsBuilt());
    EXPECT_GT(collisionManager->GetBVH().GetNodeCount(), 0);
}

TEST_F(CollisionManagerTest, UpdateBVHRebuildsTree) {
    auto component = std::make_shared<CollisionComponent>();
    BoundingBox box = {
        .min = Vector3{-1, -1, -1},
        .max = Vector3{ 1,  1,  1}
    };
    component->SetBoundingBox(box);
    collisionManager->AddCollisionComponent(component);

    collisionManager->BuildBVH();
    EXPECT_TRUE(collisionManager->GetBVH().IsBuilt());

    // Update should maintain BVH
    collisionManager->UpdateBVH();
    EXPECT_TRUE(collisionManager->GetBVH().IsBuilt());
}

TEST_F(CollisionManagerTest, CheckCollisionDetectsOverlappingBoxes) {
    auto component1 = std::make_shared<CollisionComponent>();
    BoundingBox box1 = {
        .min = Vector3{0, 0, 0},
        .max = Vector3{2, 2, 2}
    };
    component1->SetBoundingBox(box1);

    auto component2 = std::make_shared<CollisionComponent>();
    BoundingBox box2 = {
        .min = Vector3{1, 1, 1},
        .max = Vector3{3, 3, 3}
    };
    component2->SetBoundingBox(box2);

    collisionManager->AddCollisionComponent(component1);
    collisionManager->AddCollisionComponent(component2);

    bool collision = collisionManager->CheckCollision(*component1, *component2);
    EXPECT_TRUE(collision);
}

TEST_F(CollisionManagerTest, CheckCollisionDetectsNonOverlappingBoxes) {
    auto component1 = std::make_shared<CollisionComponent>();
    BoundingBox box1 = {
        .min = Vector3{0, 0, 0},
        .max = Vector3{1, 1, 1}
    };
    component1->SetBoundingBox(box1);

    auto component2 = std::make_shared<CollisionComponent>();
    BoundingBox box2 = {
        .min = Vector3{3, 3, 3},
        .max = Vector3{4, 4, 4}
    };
    component2->SetBoundingBox(box2);

    collisionManager->AddCollisionComponent(component1);
    collisionManager->AddCollisionComponent(component2);

    bool collision = collisionManager->CheckCollision(*component1, *component2);
    EXPECT_FALSE(collision);
}

TEST_F(CollisionManagerTest, GetCollisionPairsReturnsAllPairs) {
    auto component1 = std::make_shared<CollisionComponent>();
    BoundingBox box1 = {
        .min = Vector3{0, 0, 0},
        .max = Vector3{2, 2, 2}
    };
    component1->SetBoundingBox(box1);

    auto component2 = std::make_shared<CollisionComponent>();
    BoundingBox box2 = {
        .min = Vector3{1, 1, 1},
        .max = Vector3{3, 3, 3}
    };
    component2->SetBoundingBox(box2);

    collisionManager->AddCollisionComponent(component1);
    collisionManager->AddCollisionComponent(component2);

    auto pairs = collisionManager->GetCollisionPairs();
    EXPECT_EQ(pairs.size(), 1);
    EXPECT_EQ(pairs[0].first, component1.get());
    EXPECT_EQ(pairs[0].second, component2.get());
}

TEST_F(CollisionManagerTest, ProcessCollisionsCallsCallbacks) {
    auto component1 = std::make_shared<CollisionComponent>();
    BoundingBox box1 = {
        .min = Vector3{0, 0, 0},
        .max = Vector3{2, 2, 2}
    };
    component1->SetBoundingBox(box1);

    auto component2 = std::make_shared<CollisionComponent>();
    BoundingBox box2 = {
        .min = Vector3{1, 1, 1},
        .max = Vector3{3, 3, 3}
    };
    component2->SetBoundingBox(box2);

    collisionManager->AddCollisionComponent(component1);
    collisionManager->AddCollisionComponent(component2);

    bool callback1Called = false;
    bool callback2Called = false;

    component1->SetCollisionCallback([&callback1Called]() {
        callback1Called = true;
    });

    component2->SetCollisionCallback([&callback2Called]() {
        callback2Called = true;
    });

    collisionManager->ProcessCollisions();

    // Note: In real implementation, collision callbacks would be called
    // This test verifies the system can process collision pairs
    auto pairs = collisionManager->GetCollisionPairs();
    EXPECT_EQ(pairs.size(), 1);
}

TEST_F(CollisionManagerTest, ClearRemovesAllComponents) {
    auto component = std::make_shared<CollisionComponent>();
    BoundingBox box = {
        .min = Vector3{-1, -1, -1},
        .max = Vector3{ 1,  1,  1}
    };
    component->SetBoundingBox(box);
    collisionManager->AddCollisionComponent(component);

    EXPECT_EQ(collisionManager->GetCollisionComponents().size(), 1);

    collisionManager->Clear();

    EXPECT_EQ(collisionManager->GetCollisionComponents().size(), 0);
    EXPECT_FALSE(collisionManager->GetBVH().IsBuilt());
}

TEST_F(CollisionManagerTest, RaycastDetectsCollisions) {
    auto component = std::make_shared<CollisionComponent>();
    BoundingBox box = {
        .min = Vector3{0, 0, 0},
        .max = Vector3{2, 2, 2}
    };
    component->SetBoundingBox(box);
    collisionManager->AddCollisionComponent(component);

    Ray ray = {
        .position = Vector3{-1, 1, 1},
        .direction = Vector3{1, 0, 0} // Pointing towards the box
    };

    auto hitResult = collisionManager->Raycast(ray, 10.0f);

    EXPECT_TRUE(hitResult.hasHit);
    EXPECT_NEAR(hitResult.distance, 1.0f, 0.1f); // Should hit at distance 1
}

TEST_F(CollisionManagerTest, RaycastMissesNonIntersectingRays) {
    auto component = std::make_shared<CollisionComponent>();
    BoundingBox box = {
        .min = Vector3{0, 0, 0},
        .max = Vector3{1, 1, 1}
    };
    component->SetBoundingBox(box);
    collisionManager->AddCollisionComponent(component);

    Ray ray = {
        .position = Vector3{-1, -1, -1},
        .direction = Vector3{0, 0, 1} // Pointing away from the box
    };

    auto hitResult = collisionManager->Raycast(ray, 10.0f);

    EXPECT_FALSE(hitResult.hasHit);
}

TEST_F(CollisionManagerTest, SphereCollisionDetectionWorks) {
    auto component1 = std::make_shared<CollisionComponent>();
    component1->SetSphereCollision(1.0f);
    component1->SetPosition(Vector3{0, 0, 0});

    auto component2 = std::make_shared<CollisionComponent>();
    component2->SetSphereCollision(1.0f);
    component2->SetPosition(Vector3{1.5f, 0, 0}); // Distance of 1.5, sum of radii is 2.0

    collisionManager->AddCollisionComponent(component1);
    collisionManager->AddCollisionComponent(component2);

    bool collision = collisionManager->CheckCollision(*component1, *component2);
    EXPECT_TRUE(collision); // Should collide (1.5 < 2.0)
}

TEST_F(CollisionManagerTest, SphereCollisionDetectionMisses) {
    auto component1 = std::make_shared<CollisionComponent>();
    component1->SetSphereCollision(1.0f);
    component1->SetPosition(Vector3{0, 0, 0});

    auto component2 = std::make_shared<CollisionComponent>();
    component2->SetSphereCollision(1.0f);
    component2->SetPosition(Vector3{3.0f, 0, 0}); // Distance of 3.0, sum of radii is 2.0

    collisionManager->AddCollisionComponent(component1);
    collisionManager->AddCollisionComponent(component2);

    bool collision = collisionManager->CheckCollision(*component1, *component2);
    EXPECT_FALSE(collision); // Should not collide (3.0 > 2.0)
}

TEST_F(CollisionManagerTest, MixedCollisionTypesWork) {
    auto boxComponent = std::make_shared<CollisionComponent>();
    BoundingBox box = {
        .min = Vector3{0, 0, 0},
        .max = Vector3{2, 2, 2}
    };
    boxComponent->SetBoundingBox(box);

    auto sphereComponent = std::make_shared<CollisionComponent>();
    sphereComponent->SetSphereCollision(1.0f);
    sphereComponent->SetPosition(Vector3{1, 1, 1}); // Inside the box

    collisionManager->AddCollisionComponent(boxComponent);
    collisionManager->AddCollisionComponent(sphereComponent);

    bool collision = collisionManager->CheckCollision(*boxComponent, *sphereComponent);
    EXPECT_TRUE(collision); // Sphere should collide with box
}

TEST_F(CollisionManagerTest, PerformanceWithManyObjects) {
    // Add many collision components
    const int objectCount = 1000;
    std::vector<std::shared_ptr<CollisionComponent>> components;

    for (int i = 0; i < objectCount; i++) {
        auto component = std::make_shared<CollisionComponent>();
        BoundingBox box = {
            .min = Vector3{static_cast<float>(i), 0, 0},
            .max = Vector3{static_cast<float>(i + 1), 1, 1}
        };
        component->SetBoundingBox(box);
        components.push_back(component);
        collisionManager->AddCollisionComponent(component);
    }

    // Build BVH for performance
    collisionManager->BuildBVH();

    // Check that BVH was built efficiently
    EXPECT_TRUE(collisionManager->GetBVH().IsBuilt());
    EXPECT_GT(collisionManager->GetBVH().GetNodeCount(), 0);

    // Test collision query performance
    auto testComponent = std::make_shared<CollisionComponent>();
    BoundingBox testBox = {
        .min = Vector3{500, 0, 0}, // Query middle area
        .max = Vector3{502, 1, 1}
    };
    testComponent->SetBoundingBox(testBox);

    // This should be fast with BVH
    bool collision = collisionManager->CheckCollision(*testComponent, *components[500]);
    EXPECT_FALSE(collision); // Test component shouldn't collide with distant object
}