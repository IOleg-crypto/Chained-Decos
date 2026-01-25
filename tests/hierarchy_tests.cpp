#include "engine/scene/components.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene.h"
#include <gtest/gtest.h>

using namespace CHEngine;

TEST(HierarchyTest, BasicParenting)
{
    Scene scene;
    Entity parent = scene.CreateEntity("Parent");
    Entity child = scene.CreateEntity("Child");

    auto &phc = parent.AddComponent<HierarchyComponent>();
    auto &chc = child.AddComponent<HierarchyComponent>();

    chc.Parent = (entt::entity)parent;
    phc.Children.push_back((entt::entity)child);

    EXPECT_EQ(chc.Parent, (entt::entity)parent);
    EXPECT_EQ(phc.Children.size(), 1);
    EXPECT_EQ(phc.Children[0], (entt::entity)child);
}

TEST(HierarchyTest, DestroyParent)
{
    Scene scene;
    Entity parent = scene.CreateEntity("Parent");
    Entity child = scene.CreateEntity("Child");

    parent.AddComponent<HierarchyComponent>().Children.push_back((entt::entity)child);
    child.AddComponent<HierarchyComponent>().Parent = (entt::entity)parent;

    entt::entity childHandle = (entt::entity)child;
    entt::entity parentHandle = (entt::entity)parent;

    scene.DestroyEntity(parent);

    EXPECT_TRUE(scene.GetRegistry().valid(childHandle));
    auto &chc = scene.GetRegistry().get<HierarchyComponent>(childHandle);
    EXPECT_TRUE(chc.Parent == entt::null);
}

TEST(HierarchyTest, ClearParent)
{
    Scene scene;
    Entity parent = scene.CreateEntity("Parent");
    Entity child = scene.CreateEntity("Child");

    auto &phc = parent.AddComponent<HierarchyComponent>();
    auto &chc = child.AddComponent<HierarchyComponent>();

    chc.Parent = (entt::entity)parent;
    phc.Children.push_back((entt::entity)child);

    // Clear parenting
    chc.Parent = entt::null;
    auto it = std::find(phc.Children.begin(), phc.Children.end(), (entt::entity)child);
    if (it != phc.Children.end())
        phc.Children.erase(it);

    // Using true/false check because entt::null is a template thing that might confuse EXPECT_EQ
    EXPECT_TRUE(chc.Parent == entt::null);
    EXPECT_EQ(phc.Children.size(), 0);
}
