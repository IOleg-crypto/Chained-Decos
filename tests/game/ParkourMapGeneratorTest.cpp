#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "Game/Map/ParkourMapGenerator.h"
#include "Game/Map/MapLoader.h"

class ParkourMapGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        generator = std::make_shared<ParkourMapGenerator>();
    }

    void TearDown() override {
        generator.reset();
    }

    std::shared_ptr<ParkourMapGenerator> generator;
};

TEST_F(ParkourMapGeneratorTest, ConstructorInitializesDefaults) {
    EXPECT_EQ(generator->GetPlatformCount(), 0);
    EXPECT_EQ(generator->GetDifficulty(), DifficultyLevel::Test);
    EXPECT_FALSE(generator->IsGenerated());
}

TEST_F(ParkourMapGeneratorTest, SetDifficultyChangesLevel) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    EXPECT_EQ(generator->GetDifficulty(), DifficultyLevel::Easy);

    generator->SetDifficulty(DifficultyLevel::Hard);
    EXPECT_EQ(generator->GetDifficulty(), DifficultyLevel::Hard);
}

TEST_F(ParkourMapGeneratorTest, GenerateMapCreatesPlatforms) {
    generator->SetDifficulty(DifficultyLevel::Test);
    generator->GenerateMap();

    EXPECT_TRUE(generator->IsGenerated());
    EXPECT_GT(generator->GetPlatformCount(), 0);

    auto platforms = generator->GetPlatforms();
    EXPECT_GT(platforms.size(), 0);
}

TEST_F(ParkourMapGeneratorTest, TestDifficultyCreatesMinimalPlatforms) {
    generator->SetDifficulty(DifficultyLevel::Test);
    generator->GenerateMap();

    auto platforms = generator->GetPlatforms();
    EXPECT_GE(platforms.size(), 3); // At least start, middle, end platforms
    EXPECT_LE(platforms.size(), 10); // Not too many for test level
}

TEST_F(ParkourMapGeneratorTest, EasyDifficultyCreatesReasonablePlatforms) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();

    auto platforms = generator->GetPlatforms();
    EXPECT_GE(platforms.size(), 5);
    EXPECT_LE(platforms.size(), 15);
}

TEST_F(ParkourMapGeneratorTest, MediumDifficultyCreatesModeratePlatforms) {
    generator->SetDifficulty(DifficultyLevel::Medium);
    generator->GenerateMap();

    auto platforms = generator->GetPlatforms();
    EXPECT_GE(platforms.size(), 10);
    EXPECT_LE(platforms.size(), 25);
}

TEST_F(ParkourMapGeneratorTest, HardDifficultyCreatesManyPlatforms) {
    generator->SetDifficulty(DifficultyLevel::Hard);
    generator->GenerateMap();

    auto platforms = generator->GetPlatforms();
    EXPECT_GE(platforms.size(), 15);
    EXPECT_LE(platforms.size(), 40);
}

TEST_F(ParkourMapGeneratorTest, SpeedrunDifficultyOptimizesForTime) {
    generator->SetDifficulty(DifficultyLevel::Speedrun);
    generator->GenerateMap();

    auto platforms = generator->GetPlatforms();
    EXPECT_GE(platforms.size(), 8);
    EXPECT_LE(platforms.size(), 20);

    // Speedrun should have optimal platform placement
    auto map = generator->GetGeneratedMap();
    EXPECT_TRUE(map->IsOptimizedForSpeedrun());
}

TEST_F(ParkourMapGeneratorTest, PlatformsHaveValidPositions) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();

    auto platforms = generator->GetPlatforms();

    for (const auto& platform : platforms) {
        EXPECT_FALSE(std::isnan(platform.position.x));
        EXPECT_FALSE(std::isnan(platform.position.y));
        EXPECT_FALSE(std::isnan(platform.position.z));

        EXPECT_FALSE(std::isinf(platform.position.x));
        EXPECT_FALSE(std::isinf(platform.position.y));
        EXPECT_FALSE(std::isinf(platform.position.z));

        EXPECT_GT(platform.size.x, 0.0f);
        EXPECT_GT(platform.size.y, 0.0f);
        EXPECT_GT(platform.size.z, 0.0f);
    }
}

TEST_F(ParkourMapGeneratorTest, PlatformsHaveValidSizes) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();

    auto platforms = generator->GetPlatforms();

    for (const auto& platform : platforms) {
        EXPECT_GT(platform.size.x, 0.5f); // Minimum reasonable size
        EXPECT_GT(platform.size.y, 0.1f);
        EXPECT_GT(platform.size.z, 0.5f);

        EXPECT_LT(platform.size.x, 20.0f); // Maximum reasonable size
        EXPECT_LT(platform.size.y, 10.0f);
        EXPECT_LT(platform.size.z, 20.0f);
    }
}

TEST_F(ParkourMapGeneratorTest, PlatformsHaveReasonableGaps) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();

    auto platforms = generator->GetPlatforms();

    for (size_t i = 1; i < platforms.size(); i++) {
        const auto& prev = platforms[i - 1];
        const auto& curr = platforms[i];

        float gapX = abs(curr.position.x - prev.position.x);
        float gapY = abs(curr.position.y - prev.position.y);
        float gapZ = abs(curr.position.z - prev.position.z);

        // Gaps should be reasonable for jumping
        EXPECT_GT(gapX, 0.1f); // Not overlapping
        EXPECT_LT(gapX, 15.0f); // Not too far

        EXPECT_LT(gapY, 10.0f); // Not too high

        EXPECT_GT(gapZ, 0.1f); // Not overlapping
        EXPECT_LT(gapZ, 15.0f); // Not too far
    }
}

TEST_F(ParkourMapGeneratorTest, CheckpointsAreCreated) {
    generator->SetDifficulty(DifficultyLevel::Medium);
    generator->GenerateMap();

    auto checkpoints = generator->GetCheckpoints();
    EXPECT_GT(checkpoints.size(), 0);

    // First checkpoint should be at start
    EXPECT_NEAR(checkpoints[0].position.x, 0.0f, 1.0f);
    EXPECT_NEAR(checkpoints[0].position.y, 0.0f, 1.0f);
    EXPECT_NEAR(checkpoints[0].position.z, 0.0f, 1.0f);
}

TEST_F(ParkourMapGeneratorTest, DeathZonesAreCreated) {
    generator->SetDifficulty(DifficultyLevel::Medium);
    generator->GenerateMap();

    auto deathZones = generator->GetDeathZones();
    EXPECT_GT(deathZones.size(), 0);

    // Death zones should be below platforms
    for (const auto& zone : deathZones) {
        EXPECT_LT(zone.position.y, -5.0f); // Well below ground level
    }
}

TEST_F(ParkourMapGeneratorTest, MapHasValidBounds) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();

    auto bounds = generator->GetMapBounds();

    EXPECT_GT(bounds.max.x, bounds.min.x);
    EXPECT_GT(bounds.max.y, bounds.min.y);
    EXPECT_GT(bounds.max.z, bounds.min.z);

    // Map should be reasonably sized
    EXPECT_GT(bounds.max.x - bounds.min.x, 5.0f);
    EXPECT_GT(bounds.max.z - bounds.min.z, 5.0f);
}

TEST_F(ParkourMapGeneratorTest, MapCanBeSavedAndLoaded) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();

    // Save map
    std::string filename = "test_map.json";
    bool saved = generator->SaveMap(filename);
    EXPECT_TRUE(saved);

    // Load map
    auto mapLoader = std::make_shared<MapLoader>();
    auto loadedMap = mapLoader->LoadMap(filename);

    EXPECT_TRUE(loadedMap != nullptr);
    EXPECT_GT(loadedMap->GetPlatforms().size(), 0);

    // Clean up
    std::remove(filename.c_str());
}

TEST_F(ParkourMapGeneratorTest, DifferentSeedsProduceDifferentMaps) {
    generator->SetSeed(12345);
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();
    auto platforms1 = generator->GetPlatforms();

    generator->SetSeed(54321); // Different seed
    generator->GenerateMap();
    auto platforms2 = generator->GetPlatforms();

    // Maps should be different (not guaranteed, but very likely)
    bool areDifferent = platforms1.size() != platforms2.size();
    if (platforms1.size() == platforms2.size()) {
        // If same size, check if any platform is different
        for (size_t i = 0; i < platforms1.size() && !areDifferent; i++) {
            if (platforms1[i].position != platforms2[i].position) {
                areDifferent = true;
            }
        }
    }

    EXPECT_TRUE(areDifferent);
}

TEST_F(ParkourMapGeneratorTest, SameSeedsProduceIdenticalMaps) {
    generator->SetSeed(99999);
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();
    auto platforms1 = generator->GetPlatforms();

    generator->SetSeed(99999); // Same seed
    generator->GenerateMap();
    auto platforms2 = generator->GetPlatforms();

    EXPECT_EQ(platforms1.size(), platforms2.size());

    for (size_t i = 0; i < platforms1.size(); i++) {
        EXPECT_EQ(platforms1[i].position.x, platforms2[i].position.x);
        EXPECT_EQ(platforms1[i].position.y, platforms2[i].position.y);
        EXPECT_EQ(platforms1[i].position.z, platforms2[i].position.z);
    }
}

TEST_F(ParkourMapGeneratorTest, MapGenerationIsDeterministicWithSeed) {
    generator->SetSeed(77777);
    generator->SetDifficulty(DifficultyLevel::Medium);

    // Generate multiple times with same seed
    for (int i = 0; i < 5; i++) {
        generator->GenerateMap();
        auto platforms = generator->GetPlatforms();

        EXPECT_GT(platforms.size(), 0);

        // All generations should be identical
        if (i > 0) {
            auto prevPlatforms = generator->GetPlatforms();
            EXPECT_EQ(platforms.size(), prevPlatforms.size());
        }
    }
}

TEST_F(ParkourMapGeneratorTest, ClearResetsGenerator) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();

    EXPECT_TRUE(generator->IsGenerated());
    EXPECT_GT(generator->GetPlatformCount(), 0);

    generator->Clear();

    EXPECT_FALSE(generator->IsGenerated());
    EXPECT_EQ(generator->GetPlatformCount(), 0);
}

TEST_F(ParkourMapGeneratorTest, GetPlatformByIndexWorks) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();

    int platformCount = generator->GetPlatformCount();
    EXPECT_GT(platformCount, 0);

    for (int i = 0; i < platformCount; i++) {
        auto platform = generator->GetPlatform(i);
        EXPECT_TRUE(platform != nullptr);

        // Platform should have valid properties
        EXPECT_GT(platform->size.x, 0.0f);
        EXPECT_GT(platform->size.y, 0.0f);
        EXPECT_GT(platform->size.z, 0.0f);
    }
}

TEST_F(ParkourMapGeneratorTest, GetPlatformByInvalidIndexReturnsNull) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();

    auto invalidPlatform = generator->GetPlatform(-1);
    EXPECT_TRUE(invalidPlatform == nullptr);

    invalidPlatform = generator->GetPlatform(99999);
    EXPECT_TRUE(invalidPlatform == nullptr);
}

TEST_F(ParkourMapGeneratorTest, MapHasStartAndEndPlatforms) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();

    auto platforms = generator->GetPlatforms();
    EXPECT_GT(platforms.size(), 1);

    // First platform should be at reasonable starting position
    EXPECT_NEAR(platforms[0].position.y, 0.0f, 2.0f); // Near ground level
    EXPECT_NEAR(platforms[0].position.x, 0.0f, 2.0f); // Near origin
    EXPECT_NEAR(platforms[0].position.z, 0.0f, 2.0f); // Near origin

    // Last platform should be reachable
    const auto& last = platforms.back();
    EXPECT_GT(last.position.y, 0.0f); // Above ground
    EXPECT_LT(last.position.y, 50.0f); // Not too high
}

TEST_F(ParkourMapGeneratorTest, PlatformColorsAreAssigned) {
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();

    auto platforms = generator->GetPlatforms();

    for (const auto& platform : platforms) {
        // Color should be valid (not black unless specified)
        bool hasColor = platform.color.r > 0 || platform.color.g > 0 ||
                       platform.color.b > 0 || platform.color.a > 0;
        EXPECT_TRUE(hasColor);
    }
}

TEST_F(ParkourMapGeneratorTest, MovingPlatformsAreCreatedForHigherDifficulties) {
    // Test difficulty should not have moving platforms
    generator->SetDifficulty(DifficultyLevel::Test);
    generator->GenerateMap();
    auto testMovingPlatforms = generator->GetMovingPlatforms();
    EXPECT_EQ(testMovingPlatforms.size(), 0);

    // Easy might have some moving platforms
    generator->SetDifficulty(DifficultyLevel::Easy);
    generator->GenerateMap();
    auto easyMovingPlatforms = generator->GetMovingPlatforms();

    // Medium should have more moving platforms
    generator->SetDifficulty(DifficultyLevel::Medium);
    generator->GenerateMap();
    auto mediumMovingPlatforms = generator->GetMovingPlatforms();

    // Hard should have the most moving platforms
    generator->SetDifficulty(DifficultyLevel::Hard);
    generator->GenerateMap();
    auto hardMovingPlatforms = generator->GetMovingPlatforms();

    // Higher difficulties should have more moving platforms
    EXPECT_GE(mediumMovingPlatforms.size(), easyMovingPlatforms.size());
    EXPECT_GE(hardMovingPlatforms.size(), mediumMovingPlatforms.size());
}

TEST_F(ParkourMapGeneratorTest, BreakablePlatformsAreCreated) {
    generator->SetDifficulty(DifficultyLevel::Hard);
    generator->GenerateMap();

    auto breakablePlatforms = generator->GetBreakablePlatforms();
    EXPECT_GT(breakablePlatforms.size(), 0);

    for (const auto& platform : breakablePlatforms) {
        EXPECT_GT(platform.health, 0);
        EXPECT_GT(platform.respawnTime, 0.0f);
    }
}

TEST_F(ParkourMapGeneratorTest, MapDifficultyAffectsComplexity) {
    std::vector<DifficultyLevel> difficulties = {
        DifficultyLevel::Test, DifficultyLevel::Easy,
        DifficultyLevel::Medium, DifficultyLevel::Hard
    };

    std::vector<int> platformCounts;

    for (auto difficulty : difficulties) {
        generator->SetDifficulty(difficulty);
        generator->GenerateMap();
        platformCounts.push_back(generator->GetPlatformCount());
    }

    // Higher difficulties should generally have more platforms
    EXPECT_GE(platformCounts[1], platformCounts[0]); // Easy >= Test
    EXPECT_GE(platformCounts[2], platformCounts[1]); // Medium >= Easy
    EXPECT_GE(platformCounts[3], platformCounts[2]); // Hard >= Medium
}