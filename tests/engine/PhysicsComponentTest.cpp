#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "Engine/Physics/PhysicsComponent.h"
#include "Engine/Collision/CollisionComponent.h"
#include "Engine/Math/Vector3.h"

class PhysicsComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        collision = std::make_shared<CollisionComponent>();
        physics = std::make_shared<PhysicsComponent>(collision);

        // Set up basic collision box
        BoundingBox box = {
            .min = Vector3{-1, -1, -1},
            .max = Vector3{ 1,  1,  1}
        };
        collision->SetBoundingBox(box);
    }

    void TearDown() override {
        physics.reset();
        collision.reset();
    }

    std::shared_ptr<CollisionComponent> collision;
    std::shared_ptr<PhysicsComponent> physics;
};

TEST_F(PhysicsComponentTest, ConstructorInitializesDefaults) {
    EXPECT_EQ(physics->GetVelocity(), Vector3{0, 0, 0});
    EXPECT_FALSE(physics->IsGrounded());
    EXPECT_NEAR(physics->GetGravity(), -9.81f, 0.01f);
}

TEST_F(PhysicsComponentTest, SettersAndGettersWorkCorrectly) {
    Vector3 testVelocity{5, 10, 15};
    physics->SetVelocity(testVelocity);
    EXPECT_EQ(physics->GetVelocity(), testVelocity);

    physics->SetGravity(-15.0f);
    EXPECT_NEAR(physics->GetGravity(), -15.0f, 0.01f);

    physics->SetJumpForce(8.0f);
    EXPECT_NEAR(physics->GetJumpForce(), 8.0f, 0.01f);

    physics->SetMoveSpeed(12.0f);
    EXPECT_NEAR(physics->GetMoveSpeed(), 12.0f, 0.01f);
}

TEST_F(PhysicsComponentTest, GroundedStateIsManagedCorrectly) {
    EXPECT_FALSE(physics->IsGrounded());

    physics->SetGrounded(true);
    EXPECT_TRUE(physics->IsGrounded());

    physics->SetGrounded(false);
    EXPECT_FALSE(physics->IsGrounded());
}

TEST_F(PhysicsComponentTest, ApplyForceModifiesVelocity) {
    Vector3 initialVelocity = physics->GetVelocity();
    Vector3 force{10, 20, 30};

    physics->ApplyForce(force);

    Vector3 newVelocity = physics->GetVelocity();
    EXPECT_EQ(newVelocity.x, initialVelocity.x + force.x);
    EXPECT_EQ(newVelocity.y, initialVelocity.y + force.y);
    EXPECT_EQ(newVelocity.z, initialVelocity.z + force.z);
}

TEST_F(PhysicsComponentTest, UpdateAppliesGravity) {
    physics->SetVelocity(Vector3{0, 10, 0});
    physics->SetGrounded(false);

    Vector3 initialVelocity = physics->GetVelocity();
    physics->Update(0.1f); // Small time step

    Vector3 newVelocity = physics->GetVelocity();
    EXPECT_LT(newVelocity.y, initialVelocity.y); // Should decrease due to gravity
}

TEST_F(PhysicsComponentTest, UpdateDoesNotApplyGravityWhenGrounded) {
    physics->SetVelocity(Vector3{0, 10, 0});
    physics->SetGrounded(true);

    Vector3 initialVelocity = physics->GetVelocity();
    physics->Update(0.1f);

    Vector3 newVelocity = physics->GetVelocity();
    EXPECT_EQ(newVelocity.y, initialVelocity.y); // Should not change when grounded
}

TEST_F(PhysicsComponentTest, JumpAppliesForceWhenGrounded) {
    physics->SetGrounded(true);
    physics->SetJumpForce(5.0f);

    physics->Jump();

    Vector3 velocity = physics->GetVelocity();
    EXPECT_GT(velocity.y, 0.0f); // Should have upward velocity
    EXPECT_FALSE(physics->IsGrounded()); // Should no longer be grounded
}

TEST_F(PhysicsComponentTest, JumpDoesNotApplyForceWhenNotGrounded) {
    physics->SetGrounded(false);
    Vector3 initialVelocity = physics->GetVelocity();

    physics->Jump();

    Vector3 velocity = physics->GetVelocity();
    EXPECT_EQ(velocity.y, initialVelocity.y); // Should not change
}

TEST_F(PhysicsComponentTest, MoveAppliesHorizontalForce) {
    Vector3 direction{1, 0, 1}; // Diagonal movement
    physics->SetMoveSpeed(10.0f);

    physics->Move(direction);

    Vector3 velocity = physics->GetVelocity();
    EXPECT_GT(velocity.x, 0.0f); // Should have X movement
    EXPECT_GT(velocity.z, 0.0f); // Should have Z movement
    EXPECT_EQ(velocity.y, 0.0f); // Should not affect Y
}

TEST_F(PhysicsComponentTest, FrictionAffectsHorizontalMovement) {
    physics->SetVelocity(Vector3{10, 0, 10});
    physics->SetFriction(0.5f);

    physics->Update(0.1f);

    Vector3 velocity = physics->GetVelocity();
    EXPECT_LT(abs(velocity.x), 10.0f); // Should be reduced by friction
    EXPECT_LT(abs(velocity.z), 10.0f); // Should be reduced by friction
}

TEST_F(PhysicsComponentTest, AirControlAffectsMovementInAir) {
    physics->SetGrounded(false);
    physics->SetAirControl(0.1f);
    physics->SetVelocity(Vector3{5, 0, 0});

    Vector3 direction{1, 0, 0};
    physics->Move(direction);

    Vector3 velocity = physics->GetVelocity();
    EXPECT_GT(velocity.x, 5.0f); // Should be able to change direction in air
}

TEST_F(PhysicsComponentTest, CollisionCallbackIsTriggered) {
    bool collisionCalled = false;
    physics->SetCollisionCallback([&collisionCalled]() {
        collisionCalled = true;
    });

    // Simulate collision by calling the callback directly
    // (In real implementation, this would be called by CollisionSystem)
    if (physics->GetCollisionCallback()) {
        physics->GetCollisionCallback()();
    }

    EXPECT_TRUE(collisionCalled);
}

TEST_F(PhysicsComponentTest, PositionIntegrationWorksCorrectly) {
    physics->SetVelocity(Vector3{1, 2, 3});
    Vector3 initialPosition = physics->GetPosition();

    physics->Update(0.1f);

    Vector3 newPosition = physics->GetPosition();
    EXPECT_EQ(newPosition.x, initialPosition.x + 0.1f); // deltaTime * velocity.x
    EXPECT_EQ(newPosition.y, initialPosition.y + 0.2f); // deltaTime * velocity.y
    EXPECT_EQ(newPosition.z, initialPosition.z + 0.3f); // deltaTime * velocity.z
}

TEST_F(PhysicsComponentTest, MaxVelocityIsEnforced) {
    physics->SetMaxVelocity(5.0f);
    physics->SetVelocity(Vector3{10, 10, 10});

    physics->EnforceMaxVelocity();

    Vector3 velocity = physics->GetVelocity();
    EXPECT_LE(velocity.Length(), 5.0f); // Should be clamped to max velocity
}

TEST_F(PhysicsComponentTest, TerminalVelocityIsEnforced) {
    physics->SetTerminalVelocity(-20.0f);
    physics->SetVelocity(Vector3{0, -50, 0});

    physics->EnforceTerminalVelocity();

    Vector3 velocity = physics->GetVelocity();
    EXPECT_GE(velocity.y, -20.0f); // Should not exceed terminal velocity
}