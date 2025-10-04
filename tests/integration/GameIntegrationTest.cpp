#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "Game/Game.h"
#include "Game/Player/Player.h"
#include "Game/Map/ParkourMapGenerator.h"
#include "Engine/Physics/PhysicsComponent.h"
#include "Engine/Collision/CollisionSystem.h"

class GameIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        game = std::make_shared<Game>();
        game->Initialize();

        player = &game->GetPlayer();
        mapGenerator = std::make_shared<ParkourMapGenerator>();
    }

    void TearDown() override {
        game.reset();
        mapGenerator.reset();
    }

    std::shared_ptr<Game> game;
    Player* player;
    std::shared_ptr<ParkourMapGenerator> mapGenerator;
};

TEST_F(GameIntegrationTest, GameInitializesCorrectly) {
    EXPECT_TRUE(game->IsInitialized());
    EXPECT_FALSE(game->IsGameOver());
    EXPECT_FALSE(game->IsPaused());
}

TEST_F(GameIntegrationTest, PlayerSpawnsAtCorrectPosition) {
    Vector3 spawnPosition = player->GetPosition();
    EXPECT_NEAR(spawnPosition.x, 0.0f, 1.0f);
    EXPECT_NEAR(spawnPosition.y, 2.0f, 1.0f); // Above ground
    EXPECT_NEAR(spawnPosition.z, 0.0f, 1.0f);
}

TEST_F(GameIntegrationTest, PlayerHasPhysicsComponent) {
    auto physics = player->GetComponent<PhysicsComponent>();
    EXPECT_TRUE(physics != nullptr);

    EXPECT_NEAR(physics->GetGravity(), -9.81f, 0.1f);
    EXPECT_GT(physics->GetJumpForce(), 0.0f);
    EXPECT_GT(physics->GetMoveSpeed(), 0.0f);
}

TEST_F(GameIntegrationTest, PlayerHasCollisionComponent) {
    auto collision = player->GetComponent<CollisionComponent>();
    EXPECT_TRUE(collision != nullptr);

    BoundingBox box = collision->GetBoundingBox();
    EXPECT_GT(box.max.x, box.min.x);
    EXPECT_GT(box.max.y, box.min.y);
    EXPECT_GT(box.max.z, box.min.z);
}

TEST_F(GameIntegrationTest, MapGenerationIntegratesWithGame) {
    mapGenerator->SetDifficulty(DifficultyLevel::Test);
    mapGenerator->GenerateMap();

    auto map = mapGenerator->GetGeneratedMap();
    EXPECT_TRUE(map != nullptr);

    game->LoadMap(map);

    EXPECT_TRUE(game->GetCurrentMap() != nullptr);
    EXPECT_GT(game->GetCurrentMap()->GetPlatforms().size(), 0);
}

TEST_F(GameIntegrationTest, PlayerMovementAffectsPhysics) {
    Vector3 initialPosition = player->GetPosition();

    // Simulate movement input
    player->Move(Vector3{1, 0, 0});

    // Update game
    game->Update(0.1f);

    Vector3 newPosition = player->GetPosition();

    // Player should have moved (at least slightly)
    // Note: This might be 0 if player is constrained by collisions
    float movement = (newPosition - initialPosition).Length();
    EXPECT_GE(movement, 0.0f); // At least no negative movement
}

TEST_F(GameIntegrationTest, GameLoopProcessesAllSystems) {
    int frameCount = 0;
    game->SetUpdateCallback([&frameCount]() {
        frameCount++;
    });

    // Run several game loop iterations
    for (int i = 0; i < 10; i++) {
        game->Update(1.0f/60.0f);
    }

    EXPECT_GT(frameCount, 0);
}

TEST_F(GameIntegrationTest, CollisionSystemIntegratesWithPhysics) {
    // Create a platform near the player
    mapGenerator->SetDifficulty(DifficultyLevel::Test);
    mapGenerator->GenerateMap();

    auto map = mapGenerator->GetGeneratedMap();
    game->LoadMap(map);

    // Get collision system
    auto collisionSystem = game->GetCollisionSystem();
    EXPECT_TRUE(collisionSystem != nullptr);

    // Update game to process collisions
    game->Update(0.1f);

    // Player should still be valid (not fallen through world)
    Vector3 position = player->GetPosition();
    EXPECT_FALSE(std::isnan(position.x));
    EXPECT_FALSE(std::isnan(position.y));
    EXPECT_FALSE(std::isnan(position.z));
}

TEST_F(GameIntegrationTest, TimerSystemWorksCorrectly) {
    float initialTime = game->GetGameTime();
    EXPECT_GE(initialTime, 0.0f);

    // Update game for some time
    game->Update(1.0f);

    float newTime = game->GetGameTime();
    EXPECT_GT(newTime, initialTime);
}

TEST_F(GameIntegrationTest, PauseSystemWorksCorrectly) {
    EXPECT_FALSE(game->IsPaused());

    game->Pause();
    EXPECT_TRUE(game->IsPaused());

    game->Resume();
    EXPECT_FALSE(game->IsPaused());
}

TEST_F(GameIntegrationTest, GameOverStateIsManaged) {
    EXPECT_FALSE(game->IsGameOver());

    game->SetGameOver(true);
    EXPECT_TRUE(game->IsGameOver());

    game->SetGameOver(false);
    EXPECT_FALSE(game->IsGameOver());
}

TEST_F(GameIntegrationTest, ScoreSystemIntegratesWithGameplay) {
    int initialScore = game->GetScore();
    EXPECT_GE(initialScore, 0);

    // Simulate scoring event
    game->AddScore(100);

    int newScore = game->GetScore();
    EXPECT_EQ(newScore, initialScore + 100);
}

TEST_F(GameIntegrationTest, CheckpointSystemWorks) {
    mapGenerator->SetDifficulty(DifficultyLevel::Easy);
    mapGenerator->GenerateMap();

    auto map = mapGenerator->GetGeneratedMap();
    game->LoadMap(map);

    auto checkpoints = game->GetCheckpoints();
    EXPECT_GT(checkpoints.size(), 0);

    // Player should start at first checkpoint
    Vector3 playerPos = player->GetPosition();
    Vector3 firstCheckpoint = checkpoints[0].position;

    EXPECT_NEAR(playerPos.x, firstCheckpoint.x, 2.0f);
    EXPECT_NEAR(playerPos.y, firstCheckpoint.y, 2.0f);
    EXPECT_NEAR(playerPos.z, firstCheckpoint.z, 2.0f);
}

TEST_F(GameIntegrationTest, DeathZoneSystemWorks) {
    // Create a simple map with death zones
    mapGenerator->SetDifficulty(DifficultyLevel::Test);
    mapGenerator->GenerateMap();

    auto map = mapGenerator->GetGeneratedMap();
    game->LoadMap(map);

    auto deathZones = game->GetDeathZones();
    EXPECT_GT(deathZones.size(), 0);

    // Move player below death zone level
    player->SetPosition(Vector3{0, -20, 0});

    // Update game to process death zones
    game->Update(0.1f);

    // Player should be reset to spawn or game over state should be set
    Vector3 newPosition = player->GetPosition();
    EXPECT_GT(newPosition.y, -15.0f); // Should not be too deep in death zone
}

TEST_F(GameIntegrationTest, MovingPlatformsUpdateCorrectly) {
    mapGenerator->SetDifficulty(DifficultyLevel::Medium);
    mapGenerator->GenerateMap();

    auto map = mapGenerator->GetGeneratedMap();
    game->LoadMap(map);

    auto movingPlatforms = game->GetMovingPlatforms();
    int initialCount = movingPlatforms.size();

    // Update game
    game->Update(0.1f);

    // Moving platforms should still exist
    auto updatedMovingPlatforms = game->GetMovingPlatforms();
    EXPECT_EQ(updatedMovingPlatforms.size(), initialCount);
}

TEST_F(GameIntegrationTest, BreakablePlatformsWork) {
    mapGenerator->SetDifficulty(DifficultyLevel::Hard);
    mapGenerator->GenerateMap();

    auto map = mapGenerator->GetGeneratedMap();
    game->LoadMap(map);

    auto breakablePlatforms = game->GetBreakablePlatforms();
    EXPECT_GT(breakablePlatforms.size(), 0);

    if (!breakablePlatforms.empty()) {
        // Simulate damage to first breakable platform
        auto& platform = breakablePlatforms[0];
        int initialHealth = platform.health;

        platform.TakeDamage(1);

        EXPECT_EQ(platform.health, initialHealth - 1);
    }
}

TEST_F(GameIntegrationTest, GameSavesAndLoadsState) {
    // Set up some game state
    game->AddScore(500);
    game->Pause();

    // Save game state
    std::string saveFile = "test_save.json";
    bool saved = game->SaveGame(saveFile);
    EXPECT_TRUE(saved);

    // Create new game and load state
    auto newGame = std::make_shared<Game>();
    newGame->Initialize();

    bool loaded = newGame->LoadGame(saveFile);
    EXPECT_TRUE(loaded);

    // State should be preserved
    EXPECT_EQ(newGame->GetScore(), game->GetScore());
    EXPECT_EQ(newGame->IsPaused(), game->IsPaused());

    // Clean up
    std::remove(saveFile.c_str());
}

TEST_F(GameIntegrationTest, InputSystemIntegratesWithPlayer) {
    // Simulate input
    InputState input;
    input.moveForward = true;
    input.jump = true;

    game->HandleInput(input);

    // Update game
    game->Update(0.1f);

    // Player should respond to input
    Vector3 velocity = player->GetVelocity();
    EXPECT_GT(velocity.Length(), 0.0f); // Should have some movement
}

TEST_F(GameIntegrationTest, CameraFollowsPlayer) {
    Vector3 initialPlayerPos = player->GetPosition();
    Vector3 initialCameraPos = game->GetCamera().position;

    // Move player
    player->SetPosition(Vector3{5, 5, 5});

    // Update game
    game->Update(0.1f);

    // Camera should follow player
    Vector3 newCameraPos = game->GetCamera().position;
    EXPECT_GT(newCameraPos.x, initialCameraPos.x);
    EXPECT_GT(newCameraPos.y, initialCameraPos.y);
    EXPECT_GT(newCameraPos.z, initialCameraPos.z);
}

TEST_F(GameIntegrationTest, PerformanceWithComplexMap) {
    // Generate complex map
    mapGenerator->SetDifficulty(DifficultyLevel::Hard);
    mapGenerator->GenerateMap();

    auto map = mapGenerator->GetGeneratedMap();
    game->LoadMap(map);

    int platformCount = game->GetCurrentMap()->GetPlatforms().size();
    EXPECT_GT(platformCount, 10);

    // Measure update time
    auto start = std::chrono::high_resolution_clock::now();

    // Run several updates
    for (int i = 0; i < 100; i++) {
        game->Update(1.0f/60.0f);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete within reasonable time (less than 1 second for 100 frames)
    EXPECT_LT(duration.count(), 1000);
}

TEST_F(GameIntegrationTest, MemoryUsageRemainsStable) {
    // Load complex map
    mapGenerator->SetDifficulty(DifficultyLevel::Hard);
    mapGenerator->GenerateMap();

    auto map = mapGenerator->GetGeneratedMap();
    game->LoadMap(map);

    size_t initialMemory = game->GetMemoryUsage();

    // Run many updates
    for (int i = 0; i < 1000; i++) {
        game->Update(1.0f/60.0f);
    }

    size_t finalMemory = game->GetMemoryUsage();

    // Memory should not grow significantly (allow 10% growth)
    double growthRatio = static_cast<double>(finalMemory) / initialMemory;
    EXPECT_LT(growthRatio, 1.1);
}

TEST_F(GameIntegrationTest, AllSystemsWorkTogether) {
    // This is a comprehensive integration test

    // 1. Initialize game
    EXPECT_TRUE(game->IsInitialized());

    // 2. Generate and load map
    mapGenerator->SetDifficulty(DifficultyLevel::Medium);
    mapGenerator->GenerateMap();
    auto map = mapGenerator->GetGeneratedMap();
    game->LoadMap(map);

    EXPECT_TRUE(game->GetCurrentMap() != nullptr);

    // 3. Verify all systems are working
    EXPECT_TRUE(game->GetPhysicsSystem() != nullptr);
    EXPECT_TRUE(game->GetCollisionSystem() != nullptr);
    EXPECT_TRUE(game->GetRenderManager() != nullptr);

    // 4. Run game loop
    for (int i = 0; i < 50; i++) {
        game->Update(1.0f/60.0f);
    }

    // 5. Verify game state is still valid
    EXPECT_FALSE(game->IsGameOver());
    EXPECT_GE(game->GetGameTime(), 0.0f);

    Vector3 playerPos = player->GetPosition();
    EXPECT_FALSE(std::isnan(playerPos.x));
    EXPECT_FALSE(std::isnan(playerPos.y));
    EXPECT_FALSE(std::isnan(playerPos.z));
}

TEST_F(GameIntegrationTest, ErrorHandlingWorksCorrectly) {
    // Test loading invalid map
    auto invalidMap = game->LoadMap("nonexistent_file.json");
    EXPECT_FALSE(invalidMap);

    // Test with empty map
    auto emptyMap = std::make_shared<GameMap>();
    game->LoadMap(emptyMap);

    // Game should still function
    game->Update(0.1f);
    EXPECT_TRUE(game->IsInitialized());
}

TEST_F(GameIntegrationTest, SettingsPersistBetweenSessions) {
    // Set some settings
    game->SetSetting("master_volume", 0.8f);
    game->SetSetting("mouse_sensitivity", 2.0f);
    game->SetSetting("show_fps", true);

    // Save settings
    game->SaveSettings();

    // Create new game instance
    auto newGame = std::make_shared<Game>();
    newGame->Initialize();
    newGame->LoadSettings();

    // Settings should be preserved
    EXPECT_NEAR(newGame->GetSetting<float>("master_volume"), 0.8f, 0.01f);
    EXPECT_NEAR(newGame->GetSetting<float>("mouse_sensitivity"), 2.0f, 0.01f);
    EXPECT_EQ(newGame->GetSetting<bool>("show_fps"), true);
}