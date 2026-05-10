// #include <gtest/gtest.h>
// #include "engine/assets/asset_manager.h"
// #include "engine/assets/loaders/texture_loader.h"
// #include "engine/assets/loaders/model_loader.h"
// #include "engine/graphics/assets/texture_asset.h"
// #include "engine/graphics/assets/model_asset.h"
// #include "engine/core/service_locator.h"
// #include "tests/test_utils.h"

// using namespace CHEngine;

// class AssetManagerTypeSafetyTest : public ::testing::Test
// {
// protected:
//     void SetUp() override
//     {
//         auto resolver = std::make_shared<AssetPathResolver>();
//         auto registry = std::make_shared<AssetRegistry>();
//         m_AssetManager = std::make_shared<AssetManager>(resolver, registry);
        
//         m_AssetManager->RegisterLoader(AssetType::Texture, std::make_shared<TextureLoader>());
//         m_AssetManager->RegisterLoader(AssetType::Model, std::make_shared<ModelLoader>());
        
//         ServiceLocator::Register<AssetManager>(m_AssetManager.get());
//     }

//     void TearDown() override
//     {
//         ServiceLocator::Unregister<AssetManager>();
//     }

//     std::shared_ptr<AssetManager> m_AssetManager;
// };

// TEST_F(AssetManagerTypeSafetyTest, MismatchedTypeReturnsNull)
// {
//     // 1. Manually register a ModelAsset at a specific path
//     auto registry = m_AssetManager->GetRegistry();
//     std::string testPath = "test/assets/fake_texture.glb";
//     auto modelAsset = std::make_shared<ModelAsset>();
//     registry->Register(testPath, modelAsset);

//     // 2. Try to Get it as a TextureAsset
//     auto tex = m_AssetManager->Get<TextureAsset>(testPath);

//     // 3. Verify it returns nullptr instead of a mismatched pointer
//     EXPECT_EQ(tex, nullptr);
// }

// TEST_F(AssetManagerTypeSafetyTest, ValidTypeReturnsAsset)
// {
//     auto registry = m_AssetManager->GetRegistry();
//     std::string testPath = "test/assets/real_texture.png";
//     auto texAsset = std::make_shared<TextureAsset>();
//     registry->Register(testPath, texAsset);

//     auto tex = m_AssetManager->Get<TextureAsset>(testPath);
//     EXPECT_NE(tex, nullptr);
//     EXPECT_EQ(tex->GetType(), AssetType::Texture);
// }
