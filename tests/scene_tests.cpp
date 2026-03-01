#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/component_serializer.h"
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

TEST(SceneTest, FindEntityByTag)
{
    Scene scene;
    scene.CreateEntity("Entity A");
    scene.CreateEntity("Entity B");

    Entity found = scene.FindEntityByTag("Entity B");
    EXPECT_TRUE(found);
    EXPECT_EQ(found.GetName(), "Entity B");

    Entity notFound = scene.FindEntityByTag("Non-existent");
    EXPECT_FALSE(notFound);
}

TEST(SceneTest, GetEntityByUUID)
{
    Scene scene;
    Entity entity = scene.CreateEntity("Tracked Entity");
    UUID id = entity.GetUUID();

    Entity found = scene.GetEntityByUUID(id);
    EXPECT_TRUE(found);
    EXPECT_EQ(found.GetName(), "Tracked Entity");

    Entity notFound = scene.GetEntityByUUID(UUID(999999));
    EXPECT_FALSE(notFound);
}


TEST(SceneTest, CopyEntity)
{
    ComponentSerializer serializer;
    serializer.Initialize();
    
    Scene scene;
    Entity src = scene.CreateEntity("Source");
    src.AddComponent<CameraComponent>().Primary = true;
    
    Entity dst = scene.CopyEntity((entt::entity)src);
    EXPECT_TRUE(dst);
    EXPECT_EQ(dst.GetName(), "Source"); // Copied name
    EXPECT_NE(src.GetUUID(), dst.GetUUID()); // Different UUID
    EXPECT_TRUE(dst.HasComponent<CameraComponent>());
    EXPECT_TRUE(dst.GetComponent<CameraComponent>().Primary);
}
