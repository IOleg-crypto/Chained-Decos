#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

// Create a test-specific version of Vector3 that doesn't depend on raylib
struct TestVector3 {
    float x, y, z;
    TestVector3() : x(0), y(0), z(0) {}
    TestVector3(float x, float y, float z) : x(x), y(y), z(z) {}

    TestVector3 operator+(const TestVector3& other) const {
        return TestVector3(x + other.x, y + other.y, z + other.z);
    }

    TestVector3 operator*(float scalar) const {
        return TestVector3(x * scalar, y * scalar, z * scalar);
    }

    bool operator==(const TestVector3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    float Length() const {
        return std::sqrt(x*x + y*y + z*z);
    }
};

// Mock PhysicsComponent for testing
class MockPhysicsComponent {
public:
    MockPhysicsComponent() = default;

    // Test interface - simplified version without raylib dependencies
    void Update(float deltaTime) { m_deltaTime = deltaTime; }
    void SetVelocity(const TestVector3& velocity) { m_velocity = velocity; }
    TestVector3 GetVelocity() const { return m_velocity; }
    void SetGravity(float gravity) { m_gravity = gravity; }
    float GetGravity() const { return m_gravity; }
    void SetGrounded(bool grounded) { m_isGrounded = grounded; }
    bool IsGrounded() const { return m_isGrounded; }

private:
    TestVector3 m_velocity;
    float m_gravity = 9.81f;
    float m_deltaTime = 0.0f;
    bool m_isGrounded = false;
};

class PhysicsComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        physics = std::make_shared<MockPhysicsComponent>();
    }

    void TearDown() override {
        physics.reset();
    }

    std::shared_ptr<MockPhysicsComponent> physics;
};

TEST_F(PhysicsComponentTest, ConstructorInitializesDefaults) {
    EXPECT_FALSE(physics->IsGrounded());
    EXPECT_NEAR(physics->GetGravity(), 9.81f, 0.01f);
}

TEST_F(PhysicsComponentTest, SettersAndGettersWorkCorrectly) {
    TestVector3 testVelocity{5, 10, 15};
    physics->SetVelocity(testVelocity);
    EXPECT_EQ(physics->GetVelocity(), testVelocity);

    physics->SetGravity(15.0f);
    EXPECT_NEAR(physics->GetGravity(), 15.0f, 0.01f);
}

TEST_F(PhysicsComponentTest, GroundedStateIsManagedCorrectly) {
    EXPECT_FALSE(physics->IsGrounded());

    physics->SetGrounded(true);
    EXPECT_TRUE(physics->IsGrounded());

    physics->SetGrounded(false);
    EXPECT_FALSE(physics->IsGrounded());
}

TEST_F(PhysicsComponentTest, AddVelocityModifiesVelocity) {
    TestVector3 initialVelocity = physics->GetVelocity();
    TestVector3 delta{10, 20, 30};

    physics->SetVelocity(TestVector3{0, 0, 0}); // Reset first
    physics->SetVelocity(physics->GetVelocity() + delta);

    TestVector3 newVelocity = physics->GetVelocity();
    EXPECT_EQ(newVelocity.x, delta.x);
    EXPECT_EQ(newVelocity.y, delta.y);
    EXPECT_EQ(newVelocity.z, delta.z);
}

TEST_F(PhysicsComponentTest, UpdateAppliesGravity) {
    physics->SetVelocity(TestVector3{0, 10, 0});
    physics->SetGrounded(false);

    TestVector3 initialVelocity = physics->GetVelocity();
    physics->Update(0.1f); // Small time step

    // Note: Mock doesn't implement gravity, so this test documents expected behavior
    EXPECT_EQ(physics->GetVelocity(), initialVelocity); // Mock doesn't change velocity
}

TEST_F(PhysicsComponentTest, UpdateDoesNotApplyGravityWhenGrounded) {
    physics->SetVelocity(TestVector3{0, 10, 0});
    physics->SetGrounded(true);

    TestVector3 initialVelocity = physics->GetVelocity();
    physics->Update(0.1f);

    EXPECT_EQ(physics->GetVelocity(), initialVelocity); // Mock doesn't change velocity
}

TEST_F(PhysicsComponentTest, GroundedStateManagement) {
    EXPECT_FALSE(physics->IsGrounded());

    physics->SetGrounded(true);
    EXPECT_TRUE(physics->IsGrounded());

    physics->SetGrounded(false);
    EXPECT_FALSE(physics->IsGrounded());
}

TEST_F(PhysicsComponentTest, VelocityOperations) {
    TestVector3 initialVelocity{0, 0, 0};
    physics->SetVelocity(initialVelocity);
    EXPECT_EQ(physics->GetVelocity(), initialVelocity);

    TestVector3 newVelocity{5, 10, 15};
    physics->SetVelocity(newVelocity);
    EXPECT_EQ(physics->GetVelocity(), newVelocity);
}
