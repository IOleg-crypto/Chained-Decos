#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "gtest/gtest.h"

using namespace CHEngine;

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

TEST(SceneTest, EntityRenaming)
{
    Scene scene;
    Entity entity = scene.CreateEntity("Old Name");

    auto& tag = entity.GetComponent<TagComponent>().Tag;
    EXPECT_EQ(tag, "Old Name");

    tag = "New Name";
    EXPECT_EQ(entity.GetComponent<TagComponent>().Tag, "New Name");
}
