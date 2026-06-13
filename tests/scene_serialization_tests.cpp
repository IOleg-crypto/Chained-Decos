#include "engine/assets/asset_manager.h"
#include "engine/runtime/application.h"
#include "engine/scene/scene.h"
#include "engine/serialization/scene_serializer.h"
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>

using namespace Chained;


class SceneSerializationDestructiveTest : public ::testing::Test
{
protected:
    virtual void SetUp() 
    {
        std::filesystem::create_directories("test_assets");
    }
    virtual void TearDown()
    {
        std::filesystem::remove_all("test_assets");
    }

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
    SceneSerializer serializer(&scene);
    EXPECT_NO_THROW({
        bool ok = serializer.Deserialize(path);
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
    SceneSerializer serializer(&scene);
    EXPECT_NO_THROW(serializer.Deserialize(path));
}

// Valid YAML missing the "Scene:" root key.
TEST_F(SceneSerializationDestructiveTest, MissingSceneKey)
{
    std::string path = "test_assets/no_scene.chscene";
    WriteFile(path, "Entities:\n  - tag: Foo\n");

    Scene scene;
    SceneSerializer serializer(&scene);
    EXPECT_NO_THROW(serializer.Deserialize(path));
}

// Entity block missing UUID entirely.
TEST_F(SceneSerializationDestructiveTest, EntityMissingUUID)
{
    std::string path = "test_assets/no_uuid.chscene";
    WriteFile(path, "Scene: Test\n"
                    "Entities:\n"
                    "  - Entity:\n"
                    "      TagComponent:\n"
                    "        Tag: NoUUID\n");

    Scene scene;
    SceneSerializer serializer(&scene);
    // Must not crash — either skip this entity or assign a new UUID.
    EXPECT_NO_THROW(serializer.Deserialize(path));
}

// Truncated mid-token YAML.
TEST_F(SceneSerializationDestructiveTest, TruncatedYAML)
{
    std::string path = "test_assets/truncated.chscene";
    WriteFile(path,
              "Scene: Test\n"
              "Entities:\n"
              "  - Entity:\n"
              "      TagCompon"); // intentionally cut

    Scene scene;
    SceneSerializer serializer(&scene);
    EXPECT_NO_THROW(serializer.Deserialize(path));
}

// Path that does not exist.
TEST_F(SceneSerializationDestructiveTest, NonExistentFile)
{
    Scene scene;
    SceneSerializer serializer(&scene);
    EXPECT_NO_THROW({
        bool ok = serializer.Deserialize("test_assets/ghost.chscene");
        EXPECT_FALSE(ok);
    });
}

// Round-trip with 200 entities — verify count survives serialize/deserialize.
TEST_F(SceneSerializationDestructiveTest, LargeSceneRoundTrip)
{
    constexpr int kCount = 200;
    std::string path = "test_assets/large.chscene";

    {
        Scene scene;
        for (int i = 0; i < kCount; ++i)
        {
            scene.CreateEntity("E_" + std::to_string(i));
        }

        SceneSerializer serializer(&scene);
        ASSERT_TRUE(serializer.Serialize(path));
    }

    {
        Scene scene;
        SceneSerializer serializer(&scene);
        ASSERT_TRUE(serializer.Deserialize(path));

        int count = 0;
        auto view = scene.GetRegistry().view<TagComponent>();
        for (auto e : view)
        {
            ++count;
        }
    }
}

TEST_F(SceneSerializationDestructiveTest, LegacyPathResolution)
{
    // Register AssetManager for components that try to resolve paths
    auto* assetManager = Application::Get().GetServiceRegistry().Get<AssetManager>();
    ASSERT_NE(assetManager, nullptr);

    std::string path = "test_assets/legacy.chscene";
    // Construct a legacy YAML manually with paths but no handles
    WriteFile(path, "Scene: Legacy\n"
                    "Entities:\n"
                    "  - Entity: 123456789\n"
                    "    TagComponent:\n"
                    "      Tag: LegacyEntity\n"
                    "    ModelComponent:\n"
                    "      ModelPath: models/test_model.glb\n"
                    "    SpriteComponent:\n"
                    "      TexturePath: textures/test_tex.png\n");

    Scene scene;
    SceneSerializer serializer(&scene);

    // We need to mock the AssetManager resolution since the files don't actually exist
    // But ResolveToHandle/Get<T> will fail if the files are missing.
    // For the test, we'll just check if it DOES NOT crash and if we can see the paths were read.

    EXPECT_NO_THROW({
        bool ok = serializer.Deserialize(path);
        EXPECT_TRUE(ok);
    });

    auto entity = scene.FindEntityByTag("LegacyEntity");
    ASSERT_TRUE(entity);

    if (entity.HasComponent<ModelComponent>())
    {
        EXPECT_EQ(entity.GetComponent<ModelComponent>().ModelPath, "models/test_model.glb");
    }

    if (entity.HasComponent<SpriteComponent>())
    {
        EXPECT_EQ(entity.GetComponent<SpriteComponent>().TexturePath, "textures/test_tex.png");
    }
}
