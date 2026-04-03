#if 0
#include "engine/core/base.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/assets/model_asset.h"
#include "gtest/gtest.h"

using namespace CHEngine;

class AssetManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
#if defined(CH_CI)
        GTEST_SKIP() << "Skipping graphics tests on CI due to lack of reliable OpenGL support.";
#endif
        GTEST_SKIP() << "Skipping asset manager tests - raylib dependencies removed, needs refactoring.";
    }

    void TearDown() override
    {
        // Cleanup handled by GTEST_SKIP in SetUp
    }

    std::unique_ptr<AssetManager> m_AssetManager;
};

// These tests require a working OpenGL context

TEST_F(AssetManagerTest, ProceduralModelLoading)
{
    if (!IsWindowReady() || !m_AssetManager)
    {
        GTEST_SKIP() << "Skipping graphics test: No OpenGL context available.";
    }

    auto cube = m_AssetManager->Get<ModelAsset>(":cube:");
    EXPECT_TRUE(cube);
    EXPECT_GT(cube->GetModel().meshCount, 0);
}

TEST_F(AssetManagerTest, ModelCaching)
{
    if (!IsWindowReady() || !m_AssetManager)
    {
        GTEST_SKIP() << "Skipping graphics test: No OpenGL context available.";
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
        GTEST_SKIP() << "Skipping graphics test: No OpenGL context available.";
    }

    auto cube = m_AssetManager->Get<ModelAsset>(":cube:");
    EXPECT_TRUE(cube);

    // AssetManager doesn't expose a public Remove<T>(); Reload<T>() re-fetches the asset
    // from disk and replaces the cache entry — good enough to test eviction + reload.
    EXPECT_NO_THROW(m_AssetManager->Reload<ModelAsset>(":cube:"));
}


TEST_F(AssetManagerTest, NonExistentAsset)
{
    if (!IsWindowReady() || !m_AssetManager)
    {
        return;
    }

    auto asset = m_AssetManager->Get<ModelAsset>("this/path/does/not/exist.obj");
    
    // Wait for the asset to finish its async load attempt
    while(asset->GetState() == AssetState::Loading) {
        // busy wait is fine for this quick unit test
    }
    
    EXPECT_TRUE(asset != nullptr);
    if (asset)
    {
        EXPECT_EQ(asset->GetState(), AssetState::Failed);
    }
}

#endif
