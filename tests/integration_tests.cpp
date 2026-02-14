// Integration tests — headless only, no GPU/OpenGL required
#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/scene/scene_events.h"
#include "engine/core/uuid.h"
#include "engine/scene/scene.h"
#include "engine/scene/entity.h"
#include "engine/scene/components.h"
#include "engine/scene/script_registry.h"
#include "gtest/gtest.h"
#include <memory>

using namespace CHEngine;

// --- Scene + Entity Integration ---

class SceneIntegrationTest : public ::testing::Test
{
protected:
    std::shared_ptr<Scene> scene;
    void SetUp() override { scene = std::make_shared<Scene>(); }
};

TEST_F(SceneIntegrationTest, CreateAndDestroyEntity)
{
    auto entity = scene->CreateEntity("TestEntity");
    EXPECT_TRUE(entity);
    EXPECT_TRUE(entity.HasComponent<TagComponent>());
    EXPECT_EQ(entity.GetComponent<TagComponent>().Tag, "TestEntity");
    
    scene->DestroyEntity(entity);
}

TEST_F(SceneIntegrationTest, MultipleEntitiesHaveUniqueIDs)
{
    auto e1 = scene->CreateEntity("A");
    auto e2 = scene->CreateEntity("B");
    auto e3 = scene->CreateEntity("C");
    
    auto id1 = e1.GetComponent<IDComponent>().ID;
    auto id2 = e2.GetComponent<IDComponent>().ID;
    auto id3 = e3.GetComponent<IDComponent>().ID;
    
    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
}

TEST_F(SceneIntegrationTest, EntityComponentLifecycle)
{
    auto entity = scene->CreateEntity("Player");
    
    // Transform is added by default
    EXPECT_TRUE(entity.HasComponent<TransformComponent>());
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Translation = {10.0f, 20.0f, 30.0f};
    EXPECT_FLOAT_EQ(entity.GetComponent<TransformComponent>().Translation.x, 10.0f);
    
    // Add and remove a component
    entity.AddComponent<RigidBodyComponent>();
    EXPECT_TRUE(entity.HasComponent<RigidBodyComponent>());
    entity.RemoveComponent<RigidBodyComponent>();
    EXPECT_FALSE(entity.HasComponent<RigidBodyComponent>());
}

TEST_F(SceneIntegrationTest, FindEntityByTag)
{
    auto entity = scene->CreateEntity("Findable");
    
    auto found = scene->FindEntityByTag("Findable");
    EXPECT_TRUE(found);
    EXPECT_EQ(found.GetComponent<TagComponent>().Tag, "Findable");
}

// --- ScriptRegistry Integration ---

class ScriptRegistryTest : public ::testing::Test
{
protected:
    ScriptRegistry registry;
};

TEST_F(ScriptRegistryTest, RegisterAndLookup)
{
    // Register using RegisterDirect with simple lambdas
    registry.RegisterDirect(
        "TestScript",
        []() -> ScriptableEntity* { return nullptr; },  // dummy instantiate
        [](ScriptInstance*) {}                            // dummy destroy
    );
    
    auto& scripts = registry.GetScripts();
    EXPECT_EQ(scripts.count("TestScript"), 1u);
    EXPECT_NE(scripts.at("TestScript").Instantiate, nullptr);
    EXPECT_NE(scripts.at("TestScript").Destroy, nullptr);
}

TEST_F(ScriptRegistryTest, AddScriptToComponent)
{
    registry.RegisterDirect(
        "PlayerController",
        []() -> ScriptableEntity* { return nullptr; },
        [](ScriptInstance*) {}
    );
    
    NativeScriptComponent nsc;
    registry.AddScript("PlayerController", nsc);
    
    ASSERT_EQ(nsc.Scripts.size(), 1u);
    EXPECT_EQ(nsc.Scripts[0].ScriptName, "PlayerController");
    EXPECT_NE(nsc.Scripts[0].InstantiateScript, nullptr);
}

TEST_F(ScriptRegistryTest, AddUnregisteredScriptDoesNothing)
{
    NativeScriptComponent nsc;
    registry.AddScript("NonExistent", nsc);
    EXPECT_EQ(nsc.Scripts.size(), 0u);
}

TEST_F(ScriptRegistryTest, CopyFromOtherRegistry)
{
    ScriptRegistry source;
    source.RegisterDirect(
        "ScriptA",
        []() -> ScriptableEntity* { return nullptr; },
        [](ScriptInstance*) {}
    );
    
    ScriptRegistry dest;
    dest.CopyFrom(source);
    
    EXPECT_EQ(dest.GetScripts().count("ScriptA"), 1u);
}

// --- Event System Integration ---

class EventIntegrationTest : public ::testing::Test {};

TEST_F(EventIntegrationTest, SceneChangeRequestEvent)
{
    SceneChangeRequestEvent event("scenes/level2.chscene");
    EXPECT_EQ(event.GetPath(), "scenes/level2.chscene");
    EXPECT_FALSE(event.Handled);
    
    event.Handled = true;
    EXPECT_TRUE(event.Handled);
}

TEST_F(EventIntegrationTest, EventDispatcher)
{
    SceneChangeRequestEvent event("test.chscene");
    EventDispatcher dispatcher(event);
    
    bool handled = false;
    dispatcher.Dispatch<SceneChangeRequestEvent>([&handled](SceneChangeRequestEvent& e) -> bool {
        handled = true;
        return true;
    });
    
    EXPECT_TRUE(handled);
    EXPECT_TRUE(event.Handled);
}
