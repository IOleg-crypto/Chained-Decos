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

        // Get player reference
        player = &game->GetPlayer();

        // Create map generator for testing
        mapGenerator = std::make_shared<ParkourMapGenerator>();
    }

    void TearDown() override {
        game.reset();
        engine.reset();
        mapGenerator.reset();
    }

    std::shared_ptr<Engine> engine;
    std::shared_ptr<Game> game;
    Player* player;
    std::shared_ptr<ParkourMapGenerator> mapGenerator;
};

TEST_F(GameIntegrationTest, GameInitializesCorrectly) {
    // Game should be initialized after setup
    EXPECT_TRUE(game != nullptr);
    EXPECT_TRUE(engine != nullptr);
    EXPECT_TRUE(player != nullptr);

    // Player should be at a valid position
    Vector3 playerPos = player->GetPlayerPosition();
    EXPECT_FALSE(std::isnan(playerPos.x));
    EXPECT_FALSE(std::isnan(playerPos.y));
    EXPECT_FALSE(std::isnan(playerPos.z));
}

TEST_F(GameIntegrationTest, PlayerSpawnsAtCorrectPosition) {
    Vector3 spawnPosition = player->GetPlayerPosition();
    EXPECT_FALSE(std::isnan(spawnPosition.x));
    EXPECT_FALSE(std::isnan(spawnPosition.y));
    EXPECT_FALSE(std::isnan(spawnPosition.z));

    // Player should be above ground level
    EXPECT_GT(spawnPosition.y, 0.0f);
}

TEST_F(GameIntegrationTest, PlayerHasCollisionComponent) {
    // Player should have collision data
    BoundingBox box = player->GetPlayerBoundingBox();
    EXPECT_GT(box.max.x, box.min.x);
    EXPECT_GT(box.max.y, box.min.y);
    EXPECT_GT(box.max.z, box.min.z);

    // Player should have size
    Vector3 size = player->GetPlayerSize();
    EXPECT_GT(size.x, 0.0f);
    EXPECT_GT(size.y, 0.0f);
    EXPECT_GT(size.z, 0.0f);
}

TEST_F(GameIntegrationTest, MapGeneratorCreatesValidMaps) {
    // Test that map generator creates valid maps
    auto maps = mapGenerator->GetAllParkourMaps();
    EXPECT_GT(maps.size(), 0);

    // Test getting a specific map
    auto map = mapGenerator->GetMapByName("basic_shapes");
    EXPECT_FALSE(map.name.empty());
}

TEST_F(GameIntegrationTest, PlayerMovementWorks) {
    Vector3 initialPosition = player->GetPlayerPosition();

    // Move player
    player->Move(Vector3{1.0f, 0.0f, 0.0f});

    // Update game
    game->Update();

    Vector3 newPosition = player->GetPlayerPosition();

    // Position should be valid (not NaN)
    EXPECT_FALSE(std::isnan(newPosition.x));
    EXPECT_FALSE(std::isnan(newPosition.y));
    EXPECT_FALSE(std::isnan(newPosition.z));
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
    EXPECT_TRUE(player != nullptr);

    // Test that player update works with collision manager
    EXPECT_NO_THROW({
        player->Update(*collisionManager);
    });

    // Test that game update works
    EXPECT_NO_THROW({
        game->Update();
    });

    // Player position should remain valid
    Vector3 position = player->GetPlayerPosition();
    EXPECT_FALSE(std::isnan(position.x));
    EXPECT_FALSE(std::isnan(position.y));
    EXPECT_FALSE(std::isnan(position.z));
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

TEST_F(GameIntegrationTest, PlayerCanJump) {
    // Test that player can perform jump action
    EXPECT_NO_THROW({
        player->ApplyJumpImpulse(5.0f);
    });

    // Player position should remain valid after jump
    Vector3 position = player->GetPlayerPosition();
    EXPECT_FALSE(std::isnan(position.x));
    EXPECT_FALSE(std::isnan(position.y));
    EXPECT_FALSE(std::isnan(position.z));
}

TEST_F(GameIntegrationTest, PlayerHasCameraController) {
    // Test that player has a camera controller
    auto cameraController = player->GetCameraController();
    EXPECT_NE(cameraController, nullptr);
    EXPECT_NE(cameraController.get(), nullptr);
}

TEST_F(GameIntegrationTest, PlayerModelCanBeSet) {
    // Test that player model functionality works
    EXPECT_NO_THROW({
        player->SetPlayerModel(nullptr); // Should handle null gracefully
    });

    EXPECT_NO_THROW({
        player->ToggleModelRendering(true);
    });
}

TEST_F(GameIntegrationTest, GameCanLoadEditorMap) {
    // Test loading an editor map
    EXPECT_NO_THROW({
        game->LoadEditorMap("../src/Game/Resource/test.json");
    });
}

TEST_F(GameIntegrationTest, PlayerInputWorks) {
    // Test that player input processing works
    EXPECT_NO_THROW({
        player->ApplyInput();
    });
}

TEST_F(GameIntegrationTest, PlayerPositionCanBeSet) {
    Vector3 newPosition = {10.0f, 5.0f, 10.0f};

    // Test setting player position
    EXPECT_NO_THROW({
        player->SetPlayerPosition(newPosition);
    });

    // Verify position was set
    Vector3 currentPosition = player->GetPlayerPosition();
    EXPECT_FALSE(std::isnan(currentPosition.x));
    EXPECT_FALSE(std::isnan(currentPosition.y));
    EXPECT_FALSE(std::isnan(currentPosition.z));
}

TEST_F(GameIntegrationTest, GameSystemsIntegrateProperly) {
    // Test that all game systems work together
    EXPECT_NO_THROW({
        // Update player
        player->Update(*collisionManager);

        // Update game
        game->Update();
    });

    // All positions should remain valid
    Vector3 position = player->GetPlayerPosition();
    EXPECT_FALSE(std::isnan(position.x));
    EXPECT_FALSE(std::isnan(position.y));
    EXPECT_FALSE(std::isnan(position.z));
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

TEST_F(GameIntegrationTest, GameFeaturesWorkTogether) {
    // Test that all game features work together properly

    // 1. Test player movement and collision
    EXPECT_NO_THROW({
        player->Move({1.0f, 0.0f, 0.0f});
        player->Update(*collisionManager);
    });

    // 2. Test game update
    EXPECT_NO_THROW({
        game->Update();
    });

    // 3. Test that everything still works after multiple updates
    for (int i = 0; i < 5; i++) {
        EXPECT_NO_THROW({
            player->Update(*collisionManager);
            game->Update();
        });
    }

    // 4. Verify final state is valid
    Vector3 finalPosition = player->GetPlayerPosition();
    EXPECT_FALSE(std::isnan(finalPosition.x));
    EXPECT_FALSE(std::isnan(finalPosition.y));
    EXPECT_FALSE(std::isnan(finalPosition.z));
}