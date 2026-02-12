#include "engine/scene/scene.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/component_serializer.h"
#include "engine/scene/components.h"
#include "gtest/gtest.h"
#include <filesystem>

using namespace CHEngine;

TEST(SceneSerializationTest, SaveAndLoadScene)
{
    ComponentSerializer::Initialize();
    std::string testPath = "test_assets/test_scene.chscene";
    std::filesystem::create_directories("test_assets");

    UUID entityID;
    {
        Scene scene;
        Entity entity = scene.CreateEntity("Serialized Entity");
        entityID = entity.GetUUID();
        entity.AddComponent<CameraComponent>().Primary = true;
        
        SceneSerializer serializer(&scene);
        serializer.Serialize(testPath);
    }

    {
        Scene scene;
        SceneSerializer serializer(&scene);
        EXPECT_TRUE(serializer.Deserialize(testPath));

        auto view = scene.GetRegistry().view<TagComponent>();
        bool found = false;
        for (auto e : view)
        {
            Entity entity(e, &scene.GetRegistry());
            if (entity.GetUUID() == entityID)
            {
                EXPECT_EQ(entity.GetName(), "Serialized Entity");
                EXPECT_TRUE(entity.HasComponent<CameraComponent>());
                EXPECT_TRUE(entity.GetComponent<CameraComponent>().Primary);
                found = true;
            }
        }
        EXPECT_TRUE(found);
    }

    std::filesystem::remove_all("test_assets");
}
