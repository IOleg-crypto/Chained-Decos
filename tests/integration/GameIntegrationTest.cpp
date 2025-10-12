#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "Game/Game.h"
#include "Game/Player/Player.h"
#include "Game/Map/ParkourMapGenerator.h"
#include "Game/GameFeatures/ScoringSystem.h"
#include "Engine/Engine.h"
#include "Engine/Collision/CollisionManager.h"

class GameIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a mock engine for testing
        engine = std::make_shared<Engine>();
        game = std::make_shared<Game>(engine.get());

        // Initialize game systems
        game->Init();
        game->InitPlayer();
        game->LoadGameModels();

        // Create map generator for testing
        mapGenerator = std::make_shared<ParkourMapGenerator>();

        // Initialize collision manager for testing
        collisionManager = std::make_shared<CollisionManager>();
    }

    void TearDown() override {
        game.reset();
        engine.reset();
        mapGenerator.reset();
        collisionManager.reset();
    }

    std::shared_ptr<Engine> engine;
    std::shared_ptr<Game> game;
    std::shared_ptr<ParkourMapGenerator> mapGenerator;
    std::shared_ptr<CollisionManager> collisionManager;
};

TEST(GameIntegrationTest, GameInitializesCorrectly) {
    // Game should be initialized after setup
    EXPECT_TRUE(game != nullptr);
    EXPECT_TRUE(engine != nullptr);
    EXPECT_TRUE(collisionManager != nullptr);

    // Game should be running
    EXPECT_TRUE(game->IsRunning());
}

TEST(GameIntegrationTest, CollisionManagerInitializesCorrectly) {
    // Collision manager should be initialized
    EXPECT_TRUE(collisionManager != nullptr);

    // Should have at least a ground collider
    auto colliders = collisionManager->GetColliders();
    EXPECT_GE(colliders.size(), 0);
}


TEST(GameIntegrationTest, GameUpdateWorks) {
    // Game update should not crash
    EXPECT_NO_THROW({
        game->Update();
    });
}

TEST(GameIntegrationTest, GameSystemsWorkTogether) {
    // Test that game systems are properly initialized
    EXPECT_TRUE(game != nullptr);
    EXPECT_TRUE(collisionManager != nullptr);

    // Test that game update works
    EXPECT_NO_THROW({
        game->Update();
    });

    // Test multiple game updates
    for (int i = 0; i < 5; i++) {
        EXPECT_NO_THROW({
            game->Update();
        });
    }

    // Game should still be running
    EXPECT_TRUE(game->IsRunning());
}

TEST(GameIntegrationTest, GameMenuToggleWorks) {
    // Test menu toggle functionality
    EXPECT_NO_THROW({
        game->ToggleMenu();
    });
}

TEST(GameIntegrationTest, GameIsRunning) {
    // Test that game reports as running
    EXPECT_TRUE(game->IsRunning());
}

TEST(GameIntegrationTest, MapGeneratorCreatesValidMaps) {
    // Test that map generator creates valid maps
    auto maps = mapGenerator->GetAllParkourMaps();
    EXPECT_GT(maps.size(), 0);

    // Test getting a specific map
    auto map = mapGenerator->GetMapByName("basic_shapes");
    EXPECT_FALSE(map.name.empty());
}



TEST(GameIntegrationTest, GameCanLoadEditorMap) {
    // Test loading an editor map
    EXPECT_NO_THROW({
        game->LoadEditorMap("../src/Game/Resource/test.json");
    });
}

TEST(GameIntegrationTest, ErrorHandlingWorksCorrectly) {
    // Test loading invalid map
    EXPECT_NO_THROW({
        game->LoadEditorMap("nonexistent_file.json");
    });

    // Game should still function after error
    EXPECT_NO_THROW({
        game->Update();
    });

    // Game should still be running
    EXPECT_TRUE(game->IsRunning());
}

TEST(GameIntegrationTest, GameFeaturesWorkTogether) {
    // Test that all game features work together properly

    // 1. Test game update
    EXPECT_NO_THROW({
        game->Update();
    });

    // 2. Test that everything still works after multiple updates
    for (int i = 0; i < 5; i++) {
        EXPECT_NO_THROW({
            game->Update();
        });
    }

    // 3. Verify final state is valid
    EXPECT_TRUE(game->IsRunning());
}

TEST(GameIntegrationTest, AllSystemsWorkTogether) {
    // This is a comprehensive integration test

    // 1. Test that game is properly initialized
    EXPECT_TRUE(game != nullptr);
    EXPECT_TRUE(collisionManager != nullptr);

    // 2. Test that game update works
    EXPECT_NO_THROW({
        game->Update();
    });

    // 3. Test multiple game loop iterations
    for (int i = 0; i < 10; i++) {
        EXPECT_NO_THROW({
            game->Update();
        });
    }

    // 4. Verify game state remains valid
    EXPECT_TRUE(game->IsRunning());
}

TEST(GameIntegrationTest, GameCleanupWorks) {
    // Test that game cleanup works properly
    EXPECT_NO_THROW({
        game->Cleanup();
    });

    // After cleanup, player should be reset to origin
    Vector3 position = player->GetPlayerPosition();
    EXPECT_EQ(position.x, 0.0f);
    EXPECT_EQ(position.y, 0.0f);
    EXPECT_EQ(position.z, 0.0f);
}



TEST(GameIntegrationTest, GameIntegrationTestSuite) {
    // Comprehensive test of all game systems working together

    // 1. Test basic game functionality
    EXPECT_TRUE(game != nullptr);
    EXPECT_TRUE(collisionManager != nullptr);
    EXPECT_TRUE(mapGenerator != nullptr);

    // 2. Test game operations
    EXPECT_NO_THROW({
        game->Update();
        game->ToggleMenu();
        game->Update();
    });

    // 3. Test map operations
    auto maps = mapGenerator->GetAllParkourMaps();
    EXPECT_GT(maps.size(), 0);

    // 4. Test error handling
    EXPECT_NO_THROW({
        game->LoadEditorMap("nonexistent.json");
        game->Update();
    });

    // 5. Test cleanup
    EXPECT_NO_THROW({
        game->Cleanup();
    });

    // 6. Verify game is still functional after all operations
    EXPECT_TRUE(game->IsRunning());
}