#include "engine/scene/component_serializer.h"
#include "engine/scene/components.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_serializer.h"
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

using namespace CHEngine;

TEST(SceneSerializationTest, SaveAndLoadScene)
{
    std::string testPath = "test_assets/test_scene.chscene";
    std::filesystem::create_directories("test_assets");

    UUID entityID;
    {
        Scene scene;
        Entity entity = scene.CreateEntity("Serialized Entity");
        entityID = entity.GetUUID();
        entity.AddComponent<CameraComponent>().Primary = true;

        SceneSerializer s(&scene);
        s.Serialize(testPath);
    }

    {
        Scene scene;
        SceneSerializer s(&scene);
        EXPECT_TRUE(s.Deserialize(testPath));

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

// ============================================================
//  Destructive / Negative Serialization Tests
// ============================================================

class SceneSerializationDestructiveTest : public ::testing::Test
{
protected:
    void SetUp() override    { std::filesystem::create_directories("test_assets"); }
    void TearDown() override { std::filesystem::remove_all("test_assets"); }

    static void WriteFile(const std::string& path, const std::string& content)
    {
        std::ofstream f(path);
        f << content;
    }
};

// Empty file — must return false, must not throw.
TEST_F(SceneSerializationDestructiveTest, EmptyFile)
{
    std::string path = "test_assets/empty.chscene";
    WriteFile(path, "");

    Scene scene;
    SceneSerializer s(&scene);
    EXPECT_NO_THROW({
        bool ok = s.Deserialize(path);
        EXPECT_FALSE(ok);
    });
}

// Binary garbage — YAML parser must not throw unhandled exception.
TEST_F(SceneSerializationDestructiveTest, BinaryGarbage)
{
    std::string path = "test_assets/garbage.chscene";
    {
        std::ofstream f(path, std::ios::binary);
        static const unsigned char kGarbage[] = {0xFF, 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0x13, 0x37};
        f.write(reinterpret_cast<const char*>(kGarbage), sizeof(kGarbage));
    }

    Scene scene;
    SceneSerializer s(&scene);
    EXPECT_NO_THROW(s.Deserialize(path));
}

// Valid YAML missing the "Scene:" root key.
TEST_F(SceneSerializationDestructiveTest, MissingSceneKey)
{
    std::string path = "test_assets/no_scene.chscene";
    WriteFile(path, "Entities:\n  - tag: Foo\n");

    Scene scene;
    SceneSerializer s(&scene);
    EXPECT_NO_THROW(s.Deserialize(path));
}

// Entity block missing UUID entirely.
TEST_F(SceneSerializationDestructiveTest, EntityMissingUUID)
{
    std::string path = "test_assets/no_uuid.chscene";
    WriteFile(path,
        "Scene: Test\n"
        "Entities:\n"
        "  - Entity:\n"
        "      TagComponent:\n"
        "        Tag: NoUUID\n");

    Scene scene;
    SceneSerializer s(&scene);
    // Must not crash — either skip this entity or assign a new UUID.
    EXPECT_NO_THROW(s.Deserialize(path));
}

// Truncated mid-token YAML.
TEST_F(SceneSerializationDestructiveTest, TruncatedYAML)
{
    std::string path = "test_assets/truncated.chscene";
    WriteFile(path,
        "Scene: Test\n"
        "Entities:\n"
        "  - Entity:\n"
        "      TagCompon");   // intentionally cut

    Scene scene;
    SceneSerializer s(&scene);
    EXPECT_NO_THROW(s.Deserialize(path));
}

// Path that does not exist.
TEST_F(SceneSerializationDestructiveTest, NonExistentFile)
{
    Scene scene;
    SceneSerializer s(&scene);
    EXPECT_NO_THROW({
        bool ok = s.Deserialize("test_assets/ghost.chscene");
        EXPECT_FALSE(ok);
    });
}

// Round-trip with 200 entities — verify count survives serialize/deserialize.
TEST_F(SceneSerializationDestructiveTest, LargeSceneRoundTrip)
{
    constexpr int kCount = 200;
    std::string   path   = "test_assets/large.chscene";

    {
        Scene scene;
        for (int i = 0; i < kCount; ++i)
            scene.CreateEntity("E_" + std::to_string(i));

        SceneSerializer s(&scene);
        ASSERT_TRUE(s.Serialize(path));
    }

    {
        Scene scene;
        SceneSerializer s(&scene);
        ASSERT_TRUE(s.Deserialize(path));

        int count = 0;
        auto view = scene.GetRegistry().view<TagComponent>();
        for (auto e : view) ++count;
        EXPECT_EQ(count, kCount);
    }
}
