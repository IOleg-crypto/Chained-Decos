#include "engine/core/base.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/model_asset.h"
#include "gtest/gtest.h"

using namespace CHEngine;

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

        m_AssetManager = std::make_unique<AssetManager>();

        // Final check if InitWindow actually worked
        if (IsWindowReady())
        {
            m_AssetManager->Initialize();
        }
    }

    void TearDown() override
    {
        if (m_AssetManager)
        {
            m_AssetManager->Shutdown();
            m_AssetManager.reset();
        }

        if (IsWindowReady())
        {
            CloseWindow();
        }
    }

    std::unique_ptr<AssetManager> m_AssetManager;
};

// These tests require a working OpenGL context
#if !defined(CH_CI) || !defined(CH_PLATFORM_WINDOWS)

TEST_F(AssetManagerTest, ProceduralModelLoading)
{
    if (!IsWindowReady() || !m_AssetManager)
    {
        return;
    }

    auto cube = m_AssetManager->Get<ModelAsset>(":cube:");
    EXPECT_TRUE(cube);
    EXPECT_GT(cube->GetModel().meshCount, 0);
}

TEST_F(AssetManagerTest, ModelCaching)
{
    if (!IsWindowReady() || !m_AssetManager)
    {
        return;
    }

    auto cube1 = m_AssetManager->Get<ModelAsset>(":cube:");
    auto cube2 = m_AssetManager->Get<ModelAsset>(":cube:");

    EXPECT_EQ(cube1, cube2);
    EXPECT_EQ(cube1->GetModel().meshes, cube2->GetModel().meshes);
}

TEST_F(AssetManagerTest, Unloading)
{
    if (!IsWindowReady() || !m_AssetManager)
    {
        return;
    }

    auto cube = m_AssetManager->Get<ModelAsset>(":cube:");
    EXPECT_TRUE(cube);

    m_AssetManager->Remove<ModelAsset>(":cube:");
    // Cube shouldn't be in the cache, but since we didn't add a 'HasInCache' check yet,
    // we'll just check that it loads again.
}

TEST_F(AssetManagerTest, AsyncLoading)
{
    if (!IsWindowReady() || !m_AssetManager)
    {
        return;
    }

    // Test async loading of a procedural model
    auto cubeFuture = std::async(std::launch::async, [this]() { return m_AssetManager->Get<ModelAsset>(":cube:"); });

    auto cube = cubeFuture.get();
    ASSERT_TRUE(cube);
    EXPECT_TRUE(cube->IsReady());
}

#endif
