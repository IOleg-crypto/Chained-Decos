#include "engine/physics/collision/collision.h"
#include "engine/core/service_locator.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "gtest/gtest.h"

using namespace Chained;

TEST(PhysicsTest, AABBIntersection)
{
    glm::vec3 minA = {0.0f, 0.0f, 0.0f};
    glm::vec3 maxA = {1.0f, 1.0f, 1.0f};

    glm::vec3 minB = {0.5f, 0.5f, 0.5f};
    glm::vec3 maxB = {1.5f, 1.5f, 1.5f};

    // Obvious overlap
    EXPECT_TRUE(Collision::CheckAABB(minA, maxA, minB, maxB));
    EXPECT_TRUE(Collision::CheckAABB(minB, maxB, minA, maxA));

    // Touching on edge (should be true based on <= and >=)
    glm::vec3 minC = {1.0f, 0.0f, 0.0f};
    glm::vec3 maxC = {2.0f, 1.0f, 1.0f};
    EXPECT_TRUE(Collision::CheckAABB(minA, maxA, minC, maxC));

    // No overlap
    glm::vec3 minD = {1.1f, 0.0f, 0.0f};
    glm::vec3 maxD = {2.1f, 1.0f, 1.0f};
    EXPECT_FALSE(Collision::CheckAABB(minA, maxA, minD, maxD));
}

TEST(PhysicsTest, Raycast)
{
    auto scene = std::make_shared<Scene>();
    auto entity = scene->CreateEntity("Test Entity");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Translation = {0.0f, 0.0f, 5.0f};

    auto& collider = entity.AddComponent<ColliderComponent>();
    collider.Size = {1.0f, 1.0f, 1.0f};
    collider.Offset = {-0.5f, -0.5f, -0.5f}; // Center it

    auto& rb = entity.AddComponent<RigidBodyComponent>();
    rb.Type = RigidBodyComponent::BodyType::Static;

    Ray ray;
    ray.position = {0.0f, 0.0f, 0.0f};
    ray.direction = {0.0f, 0.0f, 1.0f};

    scene->OnRuntimeStart();

    RaycastResult result = ServiceLocator::Get<Physics>()->Raycast(scene.get(), ray);
    EXPECT_TRUE(result.Hit);
    EXPECT_NEAR(result.Distance, 4.5f, 0.001f);
    EXPECT_EQ(result.Entity, (entt::entity)entity);

    // Ray looking away
    ray.direction = {0.0f, 0.0f, -1.0f};
    result = ServiceLocator::Get<Physics>()->Raycast(scene.get(), ray);
    EXPECT_FALSE(result.Hit);
}

TEST(PhysicsTest, RaycastMissingCollider)
{
    auto scene = std::make_shared<Scene>();
    auto entity = scene->CreateEntity("No Collider");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Translation = {0.0f, 0.0f, 5.0f};

    // No ColliderComponent added
    auto& rb = entity.AddComponent<RigidBodyComponent>();
    rb.Type = RigidBodyComponent::BodyType::Static;

    Ray ray;
    ray.position = {0.0f, 0.0f, 0.0f};
    ray.direction = {0.0f, 0.0f, 1.0f};

    scene->OnRuntimeStart();

    RaycastResult result = ServiceLocator::Get<Physics>()->Raycast(scene.get(), ray);
    EXPECT_FALSE(result.Hit);
}

TEST(PhysicsTest, ColliderEnabledFlag)
{
    auto scene = std::make_shared<Scene>();
    auto entity = scene->CreateEntity("Collider Entity");
    auto& collider = entity.AddComponent<ColliderComponent>();
    
    // Default is true
    EXPECT_TRUE(collider.Enabled);
    
    // Set to false
    collider.Enabled = false;
    EXPECT_FALSE(collider.Enabled);
}

TEST(PhysicsTest, ContextLifecycleResetAndClear)
{
    auto scene = std::make_shared<Scene>();

    auto& context = ServiceLocator::Get<Physics>()->GetContext(scene.get());
    context.Accumulator = 0.1337f;

    bool callbackInvoked = false;
    ServiceLocator::Get<Physics>()->SetCollisionCallback(scene.get(), [&callbackInvoked](entt::entity, entt::entity) {
        callbackInvoked = true;
    });

    ServiceLocator::Get<Physics>()->ResetAccumulator(scene.get());
    EXPECT_FLOAT_EQ(ServiceLocator::Get<Physics>()->GetContext(scene.get()).Accumulator, 0.0f);
    EXPECT_TRUE((bool)ServiceLocator::Get<Physics>()->GetContext(scene.get()).CollisionCallback);

    ServiceLocator::Get<Physics>()->ClearContext(scene.get());

    auto& recreated = ServiceLocator::Get<Physics>()->GetContext(scene.get());
    EXPECT_FLOAT_EQ(recreated.Accumulator, 0.0f);
    EXPECT_FALSE((bool)recreated.CollisionCallback);
    EXPECT_FALSE(callbackInvoked);
}

TEST(PhysicsTest, RuntimeStartInitializesRigidBodyHandles)
{
    auto scene = std::make_shared<Scene>();
    auto entity = scene->CreateEntity("Physics Entity");
    auto& rigidBody = entity.AddComponent<RigidBodyComponent>();

    EXPECT_EQ(rigidBody.Handle, kInvalidPhysicsBody);

    scene->OnRuntimeStart();

    EXPECT_NE(rigidBody.Handle, kInvalidPhysicsBody);
}
