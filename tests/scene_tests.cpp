#include "engine/components.h"
#include "engine/entity.h"
#include "engine/scene.h"
#include <gtest/gtest.h>


using namespace CH;

TEST(SceneTest, CreateEntity)
{
    Scene scene;
    Entity entity = scene.CreateEntity("Test Entity");

    EXPECT_TRUE(entity);
    EXPECT_TRUE(entity.HasComponent<TagComponent>());
    EXPECT_TRUE(entity.HasComponent<TransformComponent>());
    EXPECT_EQ(entity.GetComponent<TagComponent>().Tag, "Test Entity");
}

TEST(SceneTest, DestroyEntity)
{
    Scene scene;
    Entity entity = scene.CreateEntity("To Destroy");
    entt::entity handle = entity;

    scene.DestroyEntity(entity);
    EXPECT_FALSE(scene.GetRegistry().valid(handle));
}

TEST(SceneTest, ComponentOperations)
{
    Scene scene;
    Entity entity = scene.CreateEntity();

    struct CustomComponent
    {
        int Value;
    };
    entity.AddComponent<CustomComponent>(42);

    EXPECT_TRUE(entity.HasComponent<CustomComponent>());
    EXPECT_EQ(entity.GetComponent<CustomComponent>().Value, 42);

    entity.RemoveComponent<CustomComponent>();
    EXPECT_FALSE(entity.HasComponent<CustomComponent>());
}
