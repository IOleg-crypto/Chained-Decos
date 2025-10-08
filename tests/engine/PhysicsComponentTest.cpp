#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "Engine/Physics/PhysicsComponent.h"
#include "Engine/Collision/CollisionComponent.h"
#include "Engine/Math/Vector3.h"

class PhysicsComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        physics = std::make_shared<PhysicsComponent>();
    }

    void TearDown() override {
        physics.reset();
    }

    std::shared_ptr<PhysicsComponent> physics;
};

TEST_F(PhysicsComponentTest, ConstructorInitializesDefaults) {
    //EXPECT_EQ(physics->GetVelocity(), Vector3{0, 0, 0});
    EXPECT_FALSE(physics->IsGrounded());
    EXPECT_NEAR(physics->GetGravity(), 9.81f, 0.01f);
}

TEST_F(PhysicsComponentTest, SettersAndGettersWorkCorrectly) {
    Vector3 testVelocity{5, 10, 15};
    physics->SetVelocity(testVelocity);
    EXPECT_EQ(physics->GetVelocity(), testVelocity);

    physics->SetGravity(15.0f);
    EXPECT_NEAR(physics->GetGravity(), 15.0f, 0.01f);

    physics->SetJumpStrength(8.0f);
    EXPECT_NEAR(physics->GetJumpStrength(), 8.0f, 0.01f);

    physics->SetDrag(12.0f);
    EXPECT_NEAR(physics->GetDrag(), 12.0f, 0.01f);
}

TEST_F(PhysicsComponentTest, GroundedStateIsManagedCorrectly) {
    EXPECT_FALSE(physics->IsGrounded());

    physics->SetGroundLevel(true);
    EXPECT_TRUE(physics->IsGrounded());

    physics->SetGroundLevel(false);
    EXPECT_FALSE(physics->IsGrounded());
}

TEST_F(PhysicsComponentTest, AddVelocityModifiesVelocity) {
    Vector3 initialVelocity = physics->GetVelocity();
    Vector3 delta{10, 20, 30};

    physics->AddVelocity(delta);

    Vector3 newVelocity = physics->GetVelocity();
    EXPECT_EQ(newVelocity.x, initialVelocity.x + delta.x);
    EXPECT_EQ(newVelocity.y, initialVelocity.y + delta.y);
    EXPECT_EQ(newVelocity.z, initialVelocity.z + delta.z);
}

TEST_F(PhysicsComponentTest, UpdateAppliesGravity) {
    physics->SetVelocity(Vector3{0, 10, 0});
    physics->SetGroundLevel(false);

    Vector3 initialVelocity = physics->GetVelocity();
    physics->Update(0.1f); // Small time step

    Vector3 newVelocity = physics->GetVelocity();
    EXPECT_LT(newVelocity.y, initialVelocity.y); // Should decrease due to gravity
}

TEST_F(PhysicsComponentTest, UpdateDoesNotApplyGravityWhenGrounded) {
    physics->SetVelocity(Vector3{0, 10, 0});
    physics->SetGroundLevel(true);

    Vector3 initialVelocity = physics->GetVelocity();
    physics->Update(0.1f);

    Vector3 newVelocity = physics->GetVelocity();
    EXPECT_EQ(newVelocity.y, initialVelocity.y); // Should not change when grounded
}

TEST_F(PhysicsComponentTest, JumpAppliesForceWhenGrounded) {
    physics->SetGroundLevel(true);
    physics->SetJumpStrength(5.0f);

    physics->TryJump();

    Vector3 velocity = physics->GetVelocity();
    EXPECT_GT(velocity.y, 0.0f); // Should have upward velocity
    EXPECT_FALSE(physics->IsGrounded()); // Should no longer be grounded
}

TEST_F(PhysicsComponentTest, JumpDoesNotApplyForceWhenNotGrounded) {
    physics->SetGroundLevel(false);
    Vector3 initialVelocity = physics->GetVelocity();

    physics->TryJump();

    Vector3 velocity = physics->GetVelocity();
    EXPECT_EQ(velocity.y, initialVelocity.y); // Should not change
}
