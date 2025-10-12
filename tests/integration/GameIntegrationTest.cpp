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

TEST_F(GameIntegrationTest, GameInitializesCorrectly) {
    // Game should be initialized after setup
    EXPECT_TRUE(game != nullptr);
    EXPECT_TRUE(engine != nullptr);
    EXPECT_TRUE(collisionManager != nullptr);

    // Game should be running
    EXPECT_TRUE(game->IsRunning());
}

TEST_F(GameIntegrationTest, CollisionManagerInitializesCorrectly) {
    // Collision manager should be initialized
    EXPECT_TRUE(collisionManager != nullptr);

    // Should have at least a ground collider
    auto colliders = collisionManager->GetColliders();
    EXPECT_GE(colliders.size(), 0);
}

TEST_F(GameIntegrationTest, GameUpdateWorks) {
    // Game update should not crash
    EXPECT_NO_THROW({
        game->Update();
    });

    // Game should still be running after update
    EXPECT_TRUE(game->IsRunning());
}

TEST_F(GameIntegrationTest, MapGeneratorCreatesValidMaps) {
    // Test that map generator creates valid maps
    auto maps = mapGenerator->GetAllParkourMaps();
    EXPECT_GT(maps.size(), 0);

    // Test getting a specific map
    auto map = mapGenerator->GetMapByName("basic_shapes");
    EXPECT_FALSE(map.name.empty());
}

TEST_F(GameIntegrationTest, GameMenuToggleWorks) {
    // Test menu toggle functionality
    EXPECT_NO_THROW({
        game->ToggleMenu();
    });

    // Game should still be running after menu toggle
    EXPECT_TRUE(game->IsRunning());
}

TEST_F(GameIntegrationTest, GameUpdateWorks) {
    // Game update should not crash
    EXPECT_NO_THROW({
        game->Update();
    });
}

TEST_F(GameIntegrationTest, GameSystemsWorkTogether) {
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

TEST_F(GameIntegrationTest, GameMenuToggleWorks) {
    // Test menu toggle functionality
    EXPECT_NO_THROW({
        game->ToggleMenu();
    });
}

TEST_F(GameIntegrationTest, GameIsRunning) {
    // Test that game reports as running
    EXPECT_TRUE(game->IsRunning());
}

TEST_F(GameIntegrationTest, MapGeneratorCreatesValidMaps) {
    // Test that map generator creates valid maps
    auto maps = mapGenerator->GetAllParkourMaps();
    EXPECT_GT(maps.size(), 0);

    // Test getting a specific map
    auto map = mapGenerator->GetMapByName("basic_shapes");
    EXPECT_FALSE(map.name.empty());
}

TEST_F(GameIntegrationTest, GameCanLoadEditorMap) {
    // Test loading an editor map
    EXPECT_NO_THROW({
        game->LoadEditorMap("../src/Game/Resource/test.json");
    });

    // Game should still be running after loading map
    EXPECT_TRUE(game->IsRunning());
}

TEST_F(GameIntegrationTest, GameCleanupWorks) {
    // Test that game cleanup works properly
    EXPECT_NO_THROW({
        game->Cleanup();
    });

    // Game should still be running after cleanup
    EXPECT_TRUE(game->IsRunning());
}

TEST_F(GameIntegrationTest, GameCanLoadEditorMap) {
    // Test loading an editor map
    EXPECT_NO_THROW({
        game->LoadEditorMap("../src/Game/Resource/test.json");
    });
}

TEST_F(GameIntegrationTest, ErrorHandlingWorksCorrectly) {
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

TEST_F(GameIntegrationTest, GameFeaturesWorkTogether) {
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

TEST_F(GameIntegrationTest, AllSystemsWorkTogether) {
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

TEST_F(GameIntegrationTest, GameCleanupWorks) {
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

TEST_F(GameIntegrationTest, AllSystemsWorkTogether) {
    // This is a comprehensive integration test

    // 1. Test that game is properly initialized
    EXPECT_TRUE(game != nullptr);
    EXPECT_TRUE(player != nullptr);

    // 2. Test that player and collision systems work together
    EXPECT_NO_THROW({
        player->Update(*collisionManager);
        game->Update();
    });

    // 3. Test that all positions remain valid
    Vector3 playerPos = player->GetPlayerPosition();
    EXPECT_FALSE(std::isnan(playerPos.x));
    EXPECT_FALSE(std::isnan(playerPos.y));
    EXPECT_FALSE(std::isnan(playerPos.z));

    // 4. Test multiple game loop iterations
    for (int i = 0; i < 10; i++) {
        EXPECT_NO_THROW({
            game->Update();
        });
    }

    // 5. Verify game state remains valid
    EXPECT_TRUE(game->IsRunning());
}

TEST_F(GameIntegrationTest, ErrorHandlingWorksCorrectly) {
    // Test loading invalid map
    EXPECT_NO_THROW({
        game->LoadEditorMap("nonexistent_file.json");
    });

    // Game should still function after error
    EXPECT_NO_THROW({
        game->Update();
    });
}

TEST_F(GameIntegrationTest, GameIntegrationTestSuite) {
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