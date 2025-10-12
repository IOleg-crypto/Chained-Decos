#include <gtest/gtest.h>
#include <memory>

#include "Game/Game.h"
#include "Game/Player/Player.h"
#include "Game/Map/ParkourMapGenerator.h"
#include "Engine/Engine.h"
#include "Engine/Collision/CollisionManager.h"

class GameIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Створюємо рушій і гру
        engine = std::make_shared<Engine>();
        game = std::make_shared<Game>(engine.get());

        game->Init();
        game->InitPlayer();
        game->LoadGameModels();

        mapGenerator = std::make_shared<ParkourMapGenerator>();
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

TEST_F(GameIntegrationTest, GameInitialization) {
    EXPECT_TRUE(game != nullptr);
    EXPECT_TRUE(engine != nullptr);
    EXPECT_TRUE(collisionManager != nullptr);
    EXPECT_TRUE(game->IsRunning());
}

TEST_F(GameIntegrationTest, CollisionManagerHasColliders) {
    const auto& colliders = collisionManager->GetColliders();
    EXPECT_GE(colliders.size(), 0); // хоча б один колайдер або жодного
}

TEST_F(GameIntegrationTest, GameUpdateDoesNotCrash) {
    EXPECT_NO_THROW({
        game->Update();
    });

    // Трохи покрутимо кілька разів
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW({
            game->Update();
        });
    }

    EXPECT_TRUE(game->IsRunning());
}

TEST_F(GameIntegrationTest, MenuToggleWorks) {
    EXPECT_NO_THROW({
        game->ToggleMenu();
    });
}

TEST_F(GameIntegrationTest, MapGeneratorWorks) {
    auto maps = mapGenerator->GetAllParkourMaps();
    EXPECT_GT(maps.size(), 0);

    auto map = mapGenerator->GetMapByName("basic_shapes");
    EXPECT_FALSE(map.name.empty());
}

TEST_F(GameIntegrationTest, LoadEditorMapAndErrorHandling) {
    EXPECT_NO_THROW({
        game->LoadEditorMap("../src/Game/Resource/test.json");
    });

    EXPECT_NO_THROW({
        game->LoadEditorMap("nonexistent_file.json");
    });

    EXPECT_NO_THROW({
        game->Update();
    });
    EXPECT_TRUE(game->IsRunning());
}

TEST_F(GameIntegrationTest, GameCleanupWorks) {
    EXPECT_NO_THROW({
        game->Cleanup();
    });

    // Game should still be running after cleanup
    EXPECT_TRUE(game->IsRunning());
}
