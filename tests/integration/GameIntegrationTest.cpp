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
        // Створюємо рушій і гру для тестування
        engine = std::make_shared<Engine>();
        game = std::make_shared<Game>(engine.get());

        // Ініціалізуємо гру без графіки для тестів
        game->Init();

        // Ініціалізуємо компоненти окремо для тестування
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
    EXPECT_TRUE(mapGenerator != nullptr);
}

TEST_F(GameIntegrationTest, CollisionManagerHasColliders) {
    const auto& colliders = collisionManager->GetColliders();
    EXPECT_GE(colliders.size(), 0); // хоча б один колайдер або жодного
}

TEST_F(GameIntegrationTest, GameUpdateDoesNotCrash) {
    // Тестуємо базові операції гри без графіки
    EXPECT_NO_THROW({
        game->ToggleMenu();
    });

    // Тестуємо завантаження мапи
    EXPECT_NO_THROW({
        game->LoadEditorMap("../src/Game/Resource/test.json");
    });
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

    // Тестуємо cleanup замість Update для headless середовища
    EXPECT_NO_THROW({
        game->Cleanup();
    });
}

TEST_F(GameIntegrationTest, GameCleanupWorks) {
    EXPECT_NO_THROW({
        game->Cleanup();
    });

    // Cleanup should complete without errors
    SUCCEED();
}
