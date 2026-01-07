#include "engine/core/types.h"
#include "engine/renderer/asset_manager.h"
#include <gtest/gtest.h>

using namespace CH;

class AssetManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
#if defined(CH_CI) && defined(CH_PLATFORM_WINDOWS)
        GTEST_SKIP() << "Skipping graphic tests on Windows CI due to lack of OpenGL support.";
#endif
        // Set hidden flag to avoid showing a window during tests
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        InitWindow(1, 1, "AssetManagerTest");

        // Final check if InitWindow actually worked (might fail on headless environments)
        if (!IsWindowReady())
        {
            AssetManager::Init(); // Still init logic if possible, or skip
            return;
        }

        AssetManager::Init();
    }

    void TearDown() override
    {
        if (IsWindowReady())
        {
            AssetManager::Shutdown();
            CloseWindow();
        }
    }
};

// These tests require a working OpenGL context
#if !defined(CH_CI) || !defined(CH_PLATFORM_WINDOWS)

TEST_F(AssetManagerTest, ProceduralModelLoading)
{
    if (!IsWindowReady())
        return;

    Model cube = AssetManager::LoadModel(":cube:");
    EXPECT_GT(cube.meshCount, 0);
    EXPECT_NE(cube.meshes, nullptr);

    EXPECT_TRUE(AssetManager::HasModel(":cube:"));

    Model sphere = AssetManager::LoadModel(":sphere:");
    EXPECT_GT(sphere.meshCount, 0);
    EXPECT_NE(sphere.meshes, nullptr);
}

TEST_F(AssetManagerTest, ModelCaching)
{
    if (!IsWindowReady())
        return;

    Model cube1 = AssetManager::LoadModel(":cube:");
    Model cube2 = AssetManager::LoadModel(":cube:");

    // Raylib models are structs, but they should point to the same mesh data if cached
    EXPECT_EQ(cube1.meshes, cube2.meshes);
    EXPECT_EQ(cube1.meshCount, cube2.meshCount);
}

TEST_F(AssetManagerTest, Unloading)
{
    if (!IsWindowReady())
        return;

    AssetManager::LoadModel(":cube:");
    EXPECT_TRUE(AssetManager::HasModel(":cube:"));

    AssetManager::UnloadModel(":cube:");
    EXPECT_FALSE(AssetManager::HasModel(":cube:"));
}

#endif
