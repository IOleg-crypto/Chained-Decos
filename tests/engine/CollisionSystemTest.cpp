#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>

// Test-specific types to avoid raylib dependency
struct CollisionTestVector3 {
    float x, y, z;
    CollisionTestVector3() : x(0), y(0), z(0) {}
    CollisionTestVector3(float x, float y, float z) : x(x), y(y), z(z) {}

    bool operator==(const CollisionTestVector3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct TestCollision {
    CollisionTestVector3 position;
    CollisionTestVector3 size;

    TestCollision() = default;
    TestCollision(CollisionTestVector3 pos, CollisionTestVector3 sz) : position(pos), size(sz) {}

    bool Intersects(const TestCollision& other) const {
        return !(position.x + size.x/2 < other.position.x - other.size.x/2 ||
                 position.x - size.x/2 > other.position.x + other.size.x/2 ||
                 position.y + size.y/2 < other.position.y - other.size.y/2 ||
                 position.y - size.y/2 > other.position.y + other.size.y/2 ||
                 position.z + size.z/2 < other.position.z - other.size.z/2 ||
                 position.z - size.z/2 > other.position.z + other.size.z/2);
    }
};

// Mock CollisionManager for testing
class MockCollisionManager {
public:
    void AddCollider(TestCollision collider) {
        colliders.push_back(collider);
    }

    void ClearColliders() {
        colliders.clear();
    }

    [[nodiscard]] const std::vector<TestCollision>& GetColliders() const {
        return colliders;
    }

    bool CheckCollision(const TestCollision& collider) {
        for (const auto& existing : colliders) {
            if (existing.Intersects(collider)) {
                return true;
            }
        }
        return false;
    }

private:
    std::vector<TestCollision> colliders;
};

class CollisionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        collisionManager = std::make_shared<MockCollisionManager>();
    }

    void TearDown() override {
        collisionManager.reset();
    }

    std::shared_ptr<MockCollisionManager> collisionManager;
};

TEST_F(CollisionManagerTest, ConstructorInitializesEmpty) {
    EXPECT_EQ(collisionManager->GetColliders().size(), 0);
}

TEST_F(CollisionManagerTest, AddColliderWorks) {
    TestCollision collider(CollisionTestVector3{0, 0, 0}, CollisionTestVector3{1, 1, 1});

    collisionManager->AddCollider(collider);

    EXPECT_EQ(collisionManager->GetColliders().size(), 1);
}

TEST_F(CollisionManagerTest, ClearCollidersWorks) {
    TestCollision collider(CollisionTestVector3{0, 0, 0}, CollisionTestVector3{1, 1, 1});
    collisionManager->AddCollider(collider);
    EXPECT_EQ(collisionManager->GetColliders().size(), 1);

    collisionManager->ClearColliders();
    EXPECT_EQ(collisionManager->GetColliders().size(), 0);
}

TEST_F(CollisionManagerTest, CheckCollisionDetectsOverlappingBoxes) {
    TestCollision collider1(CollisionTestVector3{0, 0, 0}, CollisionTestVector3{1, 1, 1});
    TestCollision collider2(CollisionTestVector3{0.5f, 0.5f, 0.5f}, CollisionTestVector3{1, 1, 1});

    collisionManager->AddCollider(collider1);

    bool collision = collisionManager->CheckCollision(collider2);
    EXPECT_TRUE(collision);
}

TEST_F(CollisionManagerTest, CheckCollisionDetectsNonOverlappingBoxes) {
    TestCollision collider1(CollisionTestVector3{0, 0, 0}, CollisionTestVector3{1, 1, 1});
    TestCollision collider2(CollisionTestVector3{3, 3, 3}, CollisionTestVector3{1, 1, 1});

    collisionManager->AddCollider(collider1);

    bool collision = collisionManager->CheckCollision(collider2);
    EXPECT_FALSE(collision);
}

TEST_F(CollisionManagerTest, MultipleCollidersWork) {
    TestCollision collider1(CollisionTestVector3{0, 0, 0}, CollisionTestVector3{1, 1, 1});
    TestCollision collider2(CollisionTestVector3{2, 2, 2}, CollisionTestVector3{1, 1, 1});
    TestCollision collider3(CollisionTestVector3{1, 1, 1}, CollisionTestVector3{1, 1, 1});

    collisionManager->AddCollider(collider1);
    collisionManager->AddCollider(collider2);

    EXPECT_EQ(collisionManager->GetColliders().size(), 2);

    // Test collision with non-overlapping collider
    bool collision1 = collisionManager->CheckCollision(collider3);
    EXPECT_TRUE(collision1); // Should collide with collider2

    // Test collision with overlapping collider
    TestCollision collider4(CollisionTestVector3{10, 10, 10}, CollisionTestVector3{1, 1, 1});
    bool collision2 = collisionManager->CheckCollision(collider4);
    EXPECT_FALSE(collision2); // Should not collide with any
}



