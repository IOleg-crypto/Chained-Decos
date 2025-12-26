#include <gtest/gtest.h>
#include <memory>

#include "components/physics/collision/core/collisionManager.h"
#include "core/Engine.h"
#include "project/Runtime/RuntimeApplication.h"
#include "project/Runtime/gamegui/Menu.h"
#include "project/Runtime/player/Player.h"
#include "scene/resources/map/SceneLoader.h"
#include "scene/resources/model/Model.h"

class GameIntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create dependencies
        player = std::make_shared<Player>(&CHEngine::Engine::Instance().GetAudioManager());
        collisionManager = std::make_shared<CollisionManager>();
        models = std::make_shared<ModelLoader>();
        world = std::make_shared<WorldManager>();
        menu = std::make_shared<Menu>();

        // Create game application instance
        game = std::make_shared<CHD::RuntimeApplication>(0, nullptr);

        engine = nullptr;
    }

    void TearDown() override
    {
        game.reset();
        engine.reset();
        collisionManager.reset();
        player.reset();
        models.reset();
        world.reset();
        menu.reset();
    }

    std::shared_ptr<CHEngine::Engine> engine;
    std::shared_ptr<CHD::RuntimeApplication> game;
    std::shared_ptr<CollisionManager> collisionManager;
    std::shared_ptr<Player> player;
    std::shared_ptr<ModelLoader> models;
    std::shared_ptr<WorldManager> world;
    std::shared_ptr<Menu> menu;
};

// TEST_F(GameIntegrationTest, GameInitialization) {
//     EXPECT_TRUE(game != nullptr);
//     EXPECT_TRUE(engine != nullptr);
//     EXPECT_TRUE(collisionManager != nullptr);
//     EXPECT_TRUE(mapGenerator != nullptr);
// }
//
// TEST_F(GameIntegrationTest, CollisionManagerHasColliders) {
//     const auto& colliders = collisionManager->GetColliders();
//     EXPECT_GE(colliders.size(), 0);
// }
//
////TEST_F(GameIntegrationTest, GameUpdateDoesNotCrash) {
////    EXPECT_NO_THROW({
////        game->ToggleMenu();
////    });
////
////    EXPECT_NO_THROW({
////        game->GetGameScene().objects.clear();
////    });
////}
//
////TEST_F(GameIntegrationTest, MenuToggleWorks) {
////    EXPECT_NO_THROW({
////        game->ToggleMenu();
////    });
////}
//

// TEST_F(GameIntegrationTest, LoadEditorMapAndErrorHandling) {
//
//     EXPECT_NO_THROW({
//
//         auto& gameMap = game->GetGameScene();
//         EXPECT_GE(gameMap.objects.size(), 0);
//     });
// }

// Safe tests that don't trigger graphics initialization

// TEST_F(GameIntegrationTest, GameBasicState) {
//     // Test very basic game state without accessing components that might trigger graphics
//     EXPECT_TRUE(game != nullptr);
//
//     // Test basic state queries that don't trigger initialization
//     EXPECT_NO_THROW({
//         bool isRunning = game->IsRunning();
//         bool isInitialized = game->IsInitialized();
//     });
//
//     // Test basic operations that don't require graphics
//     EXPECT_NO_THROW({
//         game->RequestExit();
//     });
// }
//
//
//  RuntimeApplication doesn't have IsRunning/IsInitialized methods
//  These tests need to be rewritten for RuntimeApplication architecture
//  TEST_F(GameIntegrationTest, GameStateQueries) {
//     // Test state queries that don't trigger graphics
//     EXPECT_NO_THROW({
//         bool isRunning = game->IsRunning();
//         bool isInitialized = game->IsInitialized();
//     });
//  }

// TEST_F(GameIntegrationTest, CollisionManagerStandalone) {
//     // Test collision manager independently (doesn't need game instance)
//     EXPECT_NO_THROW({
//         const auto& colliders = collisionManager->GetColliders();
//         EXPECT_GE(colliders.size(), 0);
//
//         collisionManager->ClearColliders();
//         EXPECT_TRUE(collisionManager->GetColliders().empty());
//     });
// }
