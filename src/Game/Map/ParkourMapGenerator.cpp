#include "ParkourMapGenerator.h"
#include <cmath>
#include <future>
#include <thread>
#include <vector>
#include <algorithm>

ParkourElement ParkourMapGenerator::CreateCube(const Vector3& position, const Vector3& size, const Color& color, bool isPlatform)
{
    return {ParkourShapeType::Cube, position, size, color, isPlatform, !isPlatform, false, 0.0f, {0, 0, 0}};
}

ParkourElement ParkourMapGenerator::CreateSphere(const Vector3& position, float radius, const Color& color, bool isPlatform)
{
    Vector3 size = {radius, radius, radius};
    return {ParkourShapeType::Sphere, position, size, color, isPlatform, !isPlatform, false, 0.0f, {0, 0, 0}};
}

ParkourElement ParkourMapGenerator::CreateCylinder(const Vector3& position, float radius, float height, const Color& color, bool isPlatform)
{
    Vector3 size = {radius, height, radius};
    return {ParkourShapeType::Cylinder, position, size, color, isPlatform, !isPlatform, false, 0.0f, {0, 0, 0}};
}

ParkourElement ParkourMapGenerator::CreatePlatform(const Vector3& position, const Vector3& size, const Color& color)
{
    return {ParkourShapeType::Plane, position, size, color, true, false, false, 0.0f, {0, 0, 0}};
}

ParkourTestMap ParkourMapGenerator::CreateBasicShapesMap()
{
    ParkourTestMap map;
    map.name = "parkour_shapes_basic";
    map.displayName = "Basic Shapes Parkour";
    map.description = "Learn parkour fundamentals with cubes, spheres, and platforms";
    map.skyColor = SKYBLUE;
    map.groundColor = DARKGREEN;
    map.difficulty = 1.5f;
    map.startPosition = {-10, 2, 0};
    map.endPosition = {50, 2, 0};

    // Ground platforms
    map.elements.push_back(CreatePlatform({0, 0, 0}, {20, 1, 20}, GRAY));
    map.elements.push_back(CreatePlatform({25, 0, 0}, {20, 1, 20}, GRAY));
    map.elements.push_back(CreatePlatform({50, 0, 0}, {20, 1, 20}, GRAY));

    // Starting platform
    map.elements.push_back(CreatePlatform({-10, 1, 0}, {5, 1, 5}, BLUE));

    // Basic cube obstacles/platforms
    map.elements.push_back(CreateCube({5, 1, 0}, {3, 2, 3}, GREEN, true));
    map.elements.push_back(CreateCube({10, 2, 0}, {2, 1, 2}, YELLOW, true));
    map.elements.push_back(CreateCube({15, 1, 0}, {3, 2, 3}, GREEN, true));

    // Sphere challenges
    map.elements.push_back(CreateSphere({20, 2, 2}, 1.5f, ORANGE, true));
    map.elements.push_back(CreateSphere({20, 2, -2}, 1.5f, ORANGE, true));

    // Cylinder platforms
    map.elements.push_back(CreateCylinder({25, 1, 0}, 2.0f, 3.0f, PURPLE, true));
    map.elements.push_back(CreateCylinder({30, 2, 0}, 1.5f, 2.0f, PINK, true));
    map.elements.push_back(CreateCylinder({35, 1, 0}, 2.0f, 3.0f, PURPLE, true));

    // Final platform
    map.elements.push_back(CreatePlatform({45, 1, 0}, {5, 1, 5}, GOLD));

    return map;
}

ParkourTestMap ParkourMapGenerator::CreateGeometricChallengeMap()
{
    ParkourTestMap map;
    map.name = "parkour_geometric";
    map.displayName = "Geometric Challenge";
    map.description = "Advanced parkour with complex geometric arrangements";
    map.skyColor = DARKBLUE;
    map.groundColor = DARKGRAY;
    map.difficulty = 3.5f;
    map.startPosition = {-5, 3, 0};
    map.endPosition = {80, 5, 0};

    // Complex geometric arrangement
    // Layered cubes forming a staircase
    for (int i = 0; i < 8; i++)
    {
        float height = 1 + i * 1.5f;
        unsigned char r = static_cast<unsigned char>(100 + i*15);
        unsigned char g = static_cast<unsigned char>(150 + i*10);
        unsigned char b = static_cast<unsigned char>(200 + i*5);
        map.elements.push_back(CreateCube({static_cast<float>(i*3), height, 0.0f}, {2.0f, 1.0f, 2.0f}, Color{r, g, b, 255}, true));
    }

    // Sphere obstacles in a pattern
    for (int i = 0; i < 5; i++)
    {
        map.elements.push_back(CreateSphere({20.0f + i*4, 3.0f + i*0.5f, 2.0f}, 1.2f, RED, false));
        map.elements.push_back(CreateSphere({20.0f + i*4, 3.0f + i*0.5f, -2.0f}, 1.2f, RED, false));
    }

    // Cylinder maze section
    for (int i = 0; i < 6; i++)
    {
        float z = i%2 == 0 ? 3.0f : -3.0f;
        map.elements.push_back(CreateCylinder({40.0f + i*2, 1.0f, z}, 1.0f, 4.0f, Color{200, 100, 100, 255}, true));
    }

    // Torus rings (using multiple cylinders to approximate)
    for (int i = 0; i < 4; i++)
    {
        float angle = i * PI/2;
        float x = 60 + cos(angle) * 3;
        float z = sin(angle) * 3;
        map.elements.push_back(CreateCylinder({x, 4, z}, 0.8f, 2.0f, Color{100, 200, 100, 255}, true));
    }

    // Final geometric structure
    map.elements.push_back(CreateCube({75, 1, 0}, {4, 8, 4}, Color{255, 215, 0, 255}, true));
    map.elements.push_back(CreatePlatform({80, 9, 0}, {6, 1, 6}, GOLD));

    return map;
}

ParkourTestMap ParkourMapGenerator::CreatePrecisionPlatformingMap()
{
    ParkourTestMap map;
    map.name = "parkour_precision";
    map.displayName = "Precision Platforming";
    map.description = "Test your precision with small platforms and tight jumps";
    map.skyColor = LIGHTGRAY;
    map.groundColor = BROWN;
    map.difficulty = 4.0f;
    map.startPosition = {0, 2, 0};
    map.endPosition = {100, 2, 0};

    // Tiny platforms requiring precise jumps
    for (int i = 0; i < 15; i++)
    {
        float x = 5 + i * 6;
        float y = 1 + sin(i * 0.5f) * 2; // Varying heights
        unsigned char g = static_cast<unsigned char>(100 + i*10);
        unsigned char b = static_cast<unsigned char>(100 + i*5);
        map.elements.push_back(CreatePlatform({x, y, 0.0f}, {1.5f, 0.5f, 1.5f}, Color{255, g, b, 255}));
    }

    // Moving precision challenges (spheres)
    for (int i = 0; i < 8; i++)
    {
        float x = 20 + i * 8;
        map.elements.push_back(CreateSphere({x, 3 + i*0.3f, 2}, 0.8f, Color{255, 150, 100, 255}, true));
    }

    // Narrow bridges
    for (int i = 0; i < 5; i++)
    {
        map.elements.push_back(CreatePlatform({50.0f + i*4, 2.0f + i*0.2f, 0.0f}, {3.0f, 0.3f, 1.0f}, Color{100, 100, 255, 255}));
    }

    // Final precision challenge - very small platforms
    map.elements.push_back(CreatePlatform({85, 1, 1}, {0.8f, 0.3f, 0.8f}, RED));
    map.elements.push_back(CreatePlatform({90, 1, -1}, {0.8f, 0.3f, 0.8f}, RED));
    map.elements.push_back(CreatePlatform({95, 1, 0}, {2, 0.5f, 2}, GOLD));

    return map;
}

ParkourTestMap ParkourMapGenerator::CreateVerticalAscensionMap()
{
    ParkourTestMap map;
    map.name = "parkour_vertical";
    map.displayName = "Vertical Ascension";
    map.description = "Climb to new heights with vertical raylib shape challenges";
    map.skyColor = DARKPURPLE;
    map.groundColor = DARKBLUE;
    map.difficulty = 4.5f;
    map.startPosition = {0, 1, 0};
    map.endPosition = {0, 50, 0};

    // Vertical climbing structure
    for (int i = 0; i < 20; i++)
    {
        float y = i * 2.5f;
        // Main climbing path
        unsigned char r = static_cast<unsigned char>(100 + i*5);
        unsigned char g = static_cast<unsigned char>(150 + i*3);
        unsigned char b = static_cast<unsigned char>(200 + i*2);
        map.elements.push_back(CreatePlatform({0.0f, y, 0.0f}, {3.0f, 0.5f, 3.0f}, Color{r, g, b, 255}));

        // Side platforms for alternative routes
        if (i % 3 == 0)
        {
            map.elements.push_back(CreatePlatform({5, y + 1, 0}, {2, 0.5f, 2}, Color{200, 100, 100, 255}));
            map.elements.push_back(CreatePlatform({-5, y + 1, 0}, {2, 0.5f, 2}, Color{100, 200, 100, 255}));
        }

        // Spherical checkpoints
        if (i % 5 == 0)
        {
            map.elements.push_back(CreateSphere({0, y + 2, 3}, 1.0f, GOLD, true));
        }
    }

    // Top platform
    map.elements.push_back(CreatePlatform({0, 48, 0}, {5, 1, 5}, Color{255, 215, 0, 255}));

    return map;
}

ParkourTestMap ParkourMapGenerator::CreateSpeedRunnersGauntletMap()
{
    ParkourTestMap map;
    map.name = "parkour_speedrun";
    map.displayName = "Speed Runner's Gauntlet";
    map.description = "Fast-paced parkour course with moving platforms";
    map.skyColor = ORANGE;
    map.groundColor = DARKBROWN;
    map.difficulty = 3.0f;
    map.startPosition = {-5, 2, 0};
    map.endPosition = {150, 2, 0};

    // Fast track with alternating platforms
    for (int i = 0; i < 30; i++)
    {
        float x = i * 5;
        float y = 1 + sin(i * 0.3f) * 1.5f; // Wavy pattern for speed
        unsigned char g = static_cast<unsigned char>(100 + i*5);
        map.elements.push_back(CreatePlatform({x, y, 0.0f}, {3.0f, 0.5f, 4.0f}, Color{255, g, 100, 255}));
    }

    // Speed boosters (ramps)
    for (int i = 0; i < 10; i++)
    {
        map.elements.push_back(CreatePlatform({20.0f + i*10, 0.5f, 5.0f}, {4.0f, 0.5f, 3.0f}, Color{255, 255, 100, 255}));
    }

    // Sphere obstacles for dodging
    for (int i = 0; i < 15; i++)
    {
        if (i % 3 == 0)
        {
            map.elements.push_back(CreateSphere({30.0f + i*6, 3.0f, 2.0f}, 1.0f, Color{255, 100, 100, 255}, false));
        }
    }

    // Final sprint section
    for (int i = 0; i < 8; i++)
    {
        map.elements.push_back(CreatePlatform({100.0f + i*3, 1.0f, 0.0f}, {2.5f, 0.3f, 3.0f}, Color{100, 255, 100, 255}));
    }

    map.elements.push_back(CreatePlatform({130, 1, 0}, {8, 2, 8}, GOLD));

    return map;
}

ParkourTestMap ParkourMapGenerator::CreateShapeTrainingGroundMap()
{
    ParkourTestMap map;
    map.name = "training_shapes";
    map.displayName = "Shape Training Ground";
    map.description = "Learn basic parkour mechanics with simple raylib shapes";
    map.skyColor = SKYBLUE;
    map.groundColor = GREEN;
    map.difficulty = 1.0f;
    map.startPosition = {0, 2, 0};
    map.endPosition = {50, 2, 0};

    // Ground
    map.elements.push_back(CreatePlatform({0, 0, 0}, {60, 1, 20}, GRAY));

    // Basic tutorial platforms
    map.elements.push_back(CreatePlatform({5, 1, 0}, {3, 1, 3}, BLUE));
    map.elements.push_back(CreatePlatform({10, 2, 0}, {3, 1, 3}, GREEN));
    map.elements.push_back(CreatePlatform({15, 3, 0}, {3, 1, 3}, YELLOW));
    map.elements.push_back(CreatePlatform({20, 2, 0}, {3, 1, 3}, GREEN));
    map.elements.push_back(CreatePlatform({25, 1, 0}, {3, 1, 3}, BLUE));

    // Shape demonstrations
    map.elements.push_back(CreateCube({30, 1, -3}, {2, 2, 2}, RED, true));
    map.elements.push_back(CreateSphere({30, 3, 3}, 1.5f, PURPLE, true));
    map.elements.push_back(CreateCylinder({35, 1, -3}, 1.0f, 3.0f, ORANGE, true));
    map.elements.push_back(CreateCylinder({35, 4, 3}, 1.0f, 3.0f, PINK, true));

    // Practice area
    for (int i = 0; i < 5; i++)
    {
        map.elements.push_back(CreatePlatform({static_cast<float>(40 + i*2), 1.0f + i*0.5f, 0.0f}, {2.0f, 0.5f, 2.0f}, Color{static_cast<unsigned char>(150 + i*20), 200, 150, 255}));
    }

    // Final goal
    map.elements.push_back(CreatePlatform({50, 1, 0}, {5, 2, 5}, GOLD));

    return map;
}

std::vector<ParkourTestMap> ParkourMapGenerator::GetAllParkourMaps()
{
    return {
        CreateBasicShapesMap(),
        CreateGeometricChallengeMap(),
        CreatePrecisionPlatformingMap(),
        CreateVerticalAscensionMap(),
        CreateSpeedRunnersGauntletMap(),
        CreateShapeTrainingGroundMap()
    };
}

// Parallel version that generates maps concurrently for better performance
std::vector<ParkourTestMap> ParkourMapGenerator::GetAllParkourMapsParallel()
{
    // Define map generation tasks
    std::vector<std::function<ParkourTestMap()>> mapGenerators = {
        []() { return ParkourMapGenerator::CreateBasicShapesMap(); },
        []() { return ParkourMapGenerator::CreateGeometricChallengeMap(); },
        []() { return ParkourMapGenerator::CreatePrecisionPlatformingMap(); },
        []() { return ParkourMapGenerator::CreateVerticalAscensionMap(); },
        []() { return ParkourMapGenerator::CreateSpeedRunnersGauntletMap(); },
        []() { return ParkourMapGenerator::CreateShapeTrainingGroundMap(); }
    };

    // Determine optimal number of threads (leave some cores free for other tasks)
    unsigned int numThreads = std::min(static_cast<unsigned int>(mapGenerators.size()),
                                      std::max(1u, std::thread::hardware_concurrency() / 2));

    std::vector<std::future<ParkourTestMap>> futures;
    std::vector<ParkourTestMap> results;

    // Launch map generation tasks in parallel
    for (unsigned int i = 0; i < mapGenerators.size(); ++i)
    {
        // Distribute tasks across available threads
        if (futures.size() < numThreads)
        {
            futures.push_back(std::async(std::launch::async, mapGenerators[i]));
        }
        else
        {
            // Wait for one task to complete before starting another
            results.push_back(futures.back().get());
            futures.pop_back();
            futures.push_back(std::async(std::launch::async, mapGenerators[i]));
        }
    }

    // Collect remaining results
    for (auto& future : futures)
    {
        results.push_back(future.get());
    }

    return results;
}

ParkourTestMap ParkourMapGenerator::GetMapByName(const std::string& name)
{
    auto allMaps = GetAllParkourMaps();
    for (const auto& map : allMaps)
    {
        if (map.name == name)
            return map;
    }

    // Return basic shapes map as default
    return CreateBasicShapesMap();
}

void ParkourMapGenerator::RenderParkourMap(const ParkourTestMap& map, Camera3D camera)
{
    // Set sky color
    ClearBackground(map.skyColor);

    BeginMode3D(camera);

    // Render all elements
    for (const auto& element : map.elements)
    {
        switch (element.type)
        {
            case ParkourShapeType::Cube:
                DrawCube(element.position, element.size.x, element.size.y, element.size.z, element.color);
                DrawCubeWires(element.position, element.size.x, element.size.y, element.size.z, BLACK);
                break;

            case ParkourShapeType::Sphere:
                DrawSphere(element.position, element.size.x, element.color);
                DrawSphereWires(element.position, element.size.x, 16, 16, BLACK);
                break;

            case ParkourShapeType::Cylinder:
                DrawCylinder(element.position, element.size.x, element.size.x, element.size.y, 16, element.color);
                DrawCylinderWires(element.position, element.size.x, element.size.x, element.size.y, 16, BLACK);
                break;

            case ParkourShapeType::Plane:
                // Draw plane as a flat cube
                DrawCube(element.position, element.size.x, element.size.y, element.size.z, element.color);
                DrawCubeWires(element.position, element.size.x, element.size.y, element.size.z, BLACK);
                break;

            case ParkourShapeType::Capsule:
                // Approximate capsule with sphere + cylinder + sphere
                DrawSphere(element.position, element.size.x, element.color);
                DrawCylinder({element.position.x, element.position.y, element.position.z}, element.size.x, element.size.x, element.size.y, 16, element.color);
                DrawSphere({element.position.x, element.position.y + element.size.y, element.position.z}, element.size.x, element.color);
                break;

            case ParkourShapeType::Torus:
                // Approximate torus with multiple cylinders
                for (int i = 0; i < 8; i++)
                {
                    float angle = i * PI/4;
                    Vector3 pos = {
                        element.position.x + static_cast<float>(cos(angle)) * element.size.x,
                        element.position.y,
                        element.position.z + static_cast<float>(sin(angle)) * element.size.x
                    };
                    DrawCylinder(pos, element.size.z, element.size.z, element.size.y, 8, element.color);
                }
                break;
        }
    }

    EndMode3D();
}