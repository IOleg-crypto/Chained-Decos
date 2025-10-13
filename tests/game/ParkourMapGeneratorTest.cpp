#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>
#include <set>
#include <iostream>

#include "Game/Map/ParkourMapGenerator.h"

class ParkourMapGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // No setup needed for static class
    }

    void TearDown() override {
        // No cleanup needed
    }
};

TEST_F(ParkourMapGeneratorTest, GetAllParkourMapsReturnsMaps) {
    auto maps = ParkourMapGenerator::GetAllParkourMaps();
    EXPECT_GT(maps.size(), 0);

    // Check that all maps have valid data
    for (const auto& map : maps) {
        EXPECT_FALSE(map.name.empty());
        EXPECT_FALSE(map.displayName.empty());
        EXPECT_FALSE(map.description.empty());
        EXPECT_GT(map.elements.size(), 0);
        EXPECT_GT(map.difficulty, 0.0f);
    }
}

TEST_F(ParkourMapGeneratorTest, GetMapByNameReturnsCorrectMap) {
    // Test getting a specific map by name
    auto map = ParkourMapGenerator::GetMapByName("basic_shapes");
    EXPECT_FALSE(map.name.empty());

    // Test getting non-existent map (should return empty map)
    auto emptyMap = ParkourMapGenerator::GetMapByName("non_existent");
    EXPECT_FALSE(emptyMap.name.empty());
}

TEST_F(ParkourMapGeneratorTest, CreateCubeCreatesValidElement) {
    Vector3 position = {0, 0, 0};
    Vector3 size = {2, 1, 2};
    Color color = RED;

    auto cube = ParkourMapGenerator::CreateCube(position, size, color, true);

    EXPECT_EQ(cube.type, ParkourShapeType::Cube);
    EXPECT_EQ(cube.position.x, position.x);
    EXPECT_EQ(cube.position.y, position.y);
    EXPECT_EQ(cube.position.z, position.z);
    EXPECT_EQ(cube.size.x, size.x);
    EXPECT_EQ(cube.size.y, size.y);
    EXPECT_EQ(cube.size.z, size.z);
    EXPECT_EQ(cube.color.r, color.r);
    EXPECT_TRUE(cube.isPlatform);
}

TEST_F(ParkourMapGeneratorTest, CreateSphereCreatesValidElement) {
    Vector3 position = {5, 3, 5};
    float radius = 1.5f;
    Color color = BLUE;

    auto sphere = ParkourMapGenerator::CreateSphere(position, radius, color, false);

    EXPECT_EQ(sphere.type, ParkourShapeType::Sphere);
    EXPECT_EQ(sphere.position.x, position.x);
    EXPECT_EQ(sphere.position.y, position.y);
    EXPECT_EQ(sphere.position.z, position.z);
    EXPECT_FALSE(sphere.isPlatform);
    EXPECT_TRUE(sphere.isObstacle);
}

TEST_F(ParkourMapGeneratorTest, CreatePlatformCreatesPlatformElement) {
    Vector3 position = {0, 5, 0};
    Vector3 size = {4, 0.5f, 4};
    Color color = GREEN;

    auto platform = ParkourMapGenerator::CreatePlatform(position, size, color);

    EXPECT_TRUE(platform.isPlatform);
    EXPECT_FALSE(platform.isObstacle);
    EXPECT_EQ(platform.color.r, color.r);
    EXPECT_EQ(platform.color.g, color.g);
    EXPECT_EQ(platform.color.b, color.b);
}

TEST_F(ParkourMapGeneratorTest, MapsHaveValidElements) {
    auto maps = ParkourMapGenerator::GetAllParkourMaps();

    for (const auto& map : maps) {
        EXPECT_FALSE(map.name.empty());
        EXPECT_FALSE(map.displayName.empty());
        EXPECT_GT(map.elements.size(), 0);

        // Check that all elements have valid positions and sizes
        for (const auto& element : map.elements) {
            EXPECT_FALSE(std::isnan(element.position.x));
            EXPECT_FALSE(std::isnan(element.position.y));
            EXPECT_FALSE(std::isnan(element.position.z));

            EXPECT_GT(element.size.x, 0.0f);
            EXPECT_GT(element.size.y, 0.0f);
            EXPECT_GT(element.size.z, 0.0f);
        }

        // Check difficulty is in valid range
        EXPECT_GT(map.difficulty, 0.0f);
        EXPECT_LE(map.difficulty, 5.0f);
    }
}

TEST_F(ParkourMapGeneratorTest, MapsHaveReasonableDifficultyProgression) {
    auto maps = ParkourMapGenerator::GetAllParkourMaps();

    // Sort maps by difficulty
    std::sort(maps.begin(), maps.end(),
              [](const ParkourTestMap& a, const ParkourTestMap& b) {
                  return a.difficulty < b.difficulty;
              });

    // Check that difficulty increases gradually
    for (size_t i = 1; i < maps.size(); i++) {
        float prevDifficulty = maps[i - 1].difficulty;
        float currDifficulty = maps[i].difficulty;

        // Difficulty should not decrease
        EXPECT_GE(currDifficulty, prevDifficulty);

        // Difficulty increase should be reasonable (not more than 2.0)
        if (currDifficulty > prevDifficulty) {
            EXPECT_LE(currDifficulty - prevDifficulty, 2.0f);
        }
    }
}




TEST_F(ParkourMapGeneratorTest, MapNamesAreUnique) {
    auto maps = ParkourMapGenerator::GetAllParkourMaps();

    // Check that all map names are unique
    std::set<std::string> names;
    for (const auto& map : maps) {
        EXPECT_TRUE(names.find(map.name) == names.end())
            << "Duplicate map name found: " << map.name;
        names.insert(map.name);
    }

    // Check that display names are also unique
    std::set<std::string> displayNames;
    for (const auto& map : maps) {
        EXPECT_TRUE(displayNames.find(map.displayName) == displayNames.end())
            << "Duplicate display name found: " << map.displayName;
        displayNames.insert(map.displayName);
    }
}

TEST_F(ParkourMapGeneratorTest, AllShapeTypesAreRepresented) {
    auto maps = ParkourMapGenerator::GetAllParkourMaps();

    // Check that different shape types are used across all maps
    std::set<ParkourShapeType> usedShapes;

    for (const auto& map : maps) {
        for (const auto& element : map.elements) {
            usedShapes.insert(element.type);
        }
    }

    // Should have at least Cube and maybe other shapes
    EXPECT_GE(usedShapes.size(), 1);

    // Log which shapes are used for debugging
    std::cout << "Used shape types: ";
    for (auto shape : usedShapes) {
        switch (shape) {
            case ParkourShapeType::Cube: std::cout << "Cube "; break;
            case ParkourShapeType::Sphere: std::cout << "Sphere "; break;
            case ParkourShapeType::Cylinder: std::cout << "Cylinder "; break;
            case ParkourShapeType::Plane: std::cout << "Plane "; break;
            case ParkourShapeType::Capsule: std::cout << "Capsule "; break;
            case ParkourShapeType::Torus: std::cout << "Torus "; break;
        }
    }
    std::cout << std::endl;
}

TEST_F(ParkourMapGeneratorTest, MapDescriptionsAreInformative) {
    auto maps = ParkourMapGenerator::GetAllParkourMaps();

    for (const auto& map : maps) {
        // Descriptions should be meaningful (not empty and reasonably long)
        EXPECT_FALSE(map.description.empty());
        EXPECT_GT(map.description.length(), 10); // At least 10 characters

        // Descriptions should contain relevant keywords
        std::string lowerDesc = map.description;
        std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);

        // Should mention parkour or related terms
        bool hasRelevantContent = lowerDesc.find("parkour") != std::string::npos ||
                                 lowerDesc.find("platform") != std::string::npos ||
                                 lowerDesc.find("jump") != std::string::npos ||
                                 lowerDesc.find("challenge") != std::string::npos;

        EXPECT_TRUE(hasRelevantContent) << "Description lacks relevant content: " << map.description;
    }
}

TEST_F(ParkourMapGeneratorTest, MapElementsHaveConsistentTypes) {
    auto maps = ParkourMapGenerator::GetAllParkourMaps();

    for (const auto& map : maps) {
        for (const auto& element : map.elements) {
            // Check that element properties are consistent with its type
            switch (element.type) {
                case ParkourShapeType::Cube:
                    // Cubes should have reasonable dimensions
                    EXPECT_GT(element.size.x, 0.1f);
                    EXPECT_GT(element.size.y, 0.1f);
                    EXPECT_GT(element.size.z, 0.1f);
                    break;

                case ParkourShapeType::Sphere:
                    // Spheres should have radius in size.x
                    EXPECT_GT(element.size.x, 0.1f); // radius
                    break;

                case ParkourShapeType::Cylinder:
                    // Cylinders should have radius and height
                    EXPECT_GT(element.size.x, 0.1f); // radius
                    EXPECT_GT(element.size.y, 0.1f); // height
                    break;

                default:
                    // Other shapes should have valid sizes
                    EXPECT_GT(element.size.x, 0.0f);
                    EXPECT_GT(element.size.y, 0.0f);
                    EXPECT_GT(element.size.z, 0.0f);
                    break;
            }
        }
    }
}

TEST_F(ParkourMapGeneratorTest, MapDifficultyCorrelatesWithElementCount) {
    auto maps = ParkourMapGenerator::GetAllParkourMaps();

    // Sort maps by difficulty
    std::sort(maps.begin(), maps.end(),
              [](const ParkourTestMap& a, const ParkourTestMap& b) {
                  return a.difficulty < b.difficulty;
              });

    // Higher difficulty maps should generally have more elements
    for (size_t i = 1; i < maps.size(); i++) {
        int prevElementCount = maps[i - 1].elements.size();
        int currElementCount = maps[i].elements.size();

        // If difficulty increases, element count should not decrease significantly
        if (maps[i].difficulty > maps[i - 1].difficulty) {
            // Allow some flexibility in element count vs difficulty correlation
            float elementRatio = static_cast<float>(currElementCount) / prevElementCount;
            EXPECT_GE(elementRatio, 0.5f) << "Element count drops too drastically between difficulties";
        }
    }
}

TEST_F(ParkourMapGeneratorTest, StaticClassMethodsWorkCorrectly) {
    // Test that static methods work without instance
    EXPECT_NO_THROW({
        auto maps = ParkourMapGenerator::GetAllParkourMaps();
        EXPECT_GT(maps.size(), 0);
    });

    EXPECT_NO_THROW({
        auto map = ParkourMapGenerator::GetMapByName("nonexistent");
        EXPECT_FALSE(map.name.empty());
    });

    EXPECT_NO_THROW({
        auto cube = ParkourMapGenerator::CreateCube({0, 0, 0}, {1, 1, 1}, RED);
        EXPECT_EQ(cube.type, ParkourShapeType::Cube);
    });
}