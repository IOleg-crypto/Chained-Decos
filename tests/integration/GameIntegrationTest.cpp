#include <gtest/gtest.h>
#include <memory>

#include "Game/Game.h"
#include "Game/Player/Player.h"
#include "Engine/Engine.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include "Engine/World/World.h"
#include "Game/Menu/Menu.h"
#include "Game/Map/MapLoader.h"

class GameIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create dependencies
        player = std::make_shared<Player>();
        collisionManager = std::make_shared<CollisionManager>();
        models = std::make_shared<ModelLoader>();
        world = std::make_shared<WorldManager>();
        menu = std::make_shared<Menu>();

        // Create game instance
        game = std::make_shared<Game>();

        engine = nullptr;
    }

    void TearDown() override {
        game.reset();
        engine.reset();
        collisionManager.reset();
        player.reset();
        models.reset();
        world.reset();
        menu.reset();
    }

    std::shared_ptr<Engine> engine;
    std::shared_ptr<Game> game;
    std::shared_ptr<CollisionManager> collisionManager;
    std::shared_ptr<Player> player;
    std::shared_ptr<ModelLoader> models;
    std::shared_ptr<WorldManager> world;
    std::shared_ptr<Menu> menu;
};

//TEST_F(GameIntegrationTest, GameInitialization) {
//    EXPECT_TRUE(game != nullptr);
//    EXPECT_TRUE(engine != nullptr);
//    EXPECT_TRUE(collisionManager != nullptr);
//    EXPECT_TRUE(mapGenerator != nullptr);
//}
//
//TEST_F(GameIntegrationTest, CollisionManagerHasColliders) {
//    const auto& colliders = collisionManager->GetColliders();
//    EXPECT_GE(colliders.size(), 0);
//}
//
////TEST_F(GameIntegrationTest, GameUpdateDoesNotCrash) {
////    EXPECT_NO_THROW({
////        game->ToggleMenu();
////    });
////
////    EXPECT_NO_THROW({
////        game->GetGameMap().objects.clear();
////    });
////}
//
////TEST_F(GameIntegrationTest, MenuToggleWorks) {
////    EXPECT_NO_THROW({
////        game->ToggleMenu();
////    });
////}
//

//TEST_F(GameIntegrationTest, LoadEditorMapAndErrorHandling) {
//
//    EXPECT_NO_THROW({
//
//        auto& gameMap = game->GetGameMap();
//        EXPECT_GE(gameMap.objects.size(), 0);
//    });
//}

// Safe tests that don't trigger graphics initialization

//TEST_F(GameIntegrationTest, GameBasicState) {
//    // Test very basic game state without accessing components that might trigger graphics
//    EXPECT_TRUE(game != nullptr);
//
//    // Test basic state queries that don't trigger initialization
//    EXPECT_NO_THROW({
//        bool isRunning = game->IsRunning();
//        bool isInitialized = game->IsInitialized();
//    });
//
//    // Test basic operations that don't require graphics
//    EXPECT_NO_THROW({
//        game->RequestExit();
//    });
//}
//
//
TEST_F(GameIntegrationTest, GameStateQueries) {
   // Test state queries that don't trigger graphics
   EXPECT_NO_THROW({
       bool isRunning = game->IsRunning();
       bool isInitialized = game->IsInitialized();
   });
}


//TEST_F(GameIntegrationTest, CollisionManagerStandalone) {
//    // Test collision manager independently (doesn't need game instance)
//    EXPECT_NO_THROW({
//        const auto& colliders = collisionManager->GetColliders();
//        EXPECT_GE(colliders.size(), 0);
//
//        collisionManager->ClearColliders();
//        EXPECT_TRUE(collisionManager->GetColliders().empty());
//    });
//}

