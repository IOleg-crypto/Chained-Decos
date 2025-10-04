#include <benchmark/benchmark.h>
#include <memory>

#include "Engine/Physics/PhysicsComponent.h"
#include "Engine/Collision/CollisionSystem.h"
#include "Engine/Collision/CollisionComponent.h"
#include "Game/Map/ParkourMapGenerator.h"
#include "Game/Game.h"

static void BM_PhysicsComponentUpdate(benchmark::State& state) {
    auto collision = std::make_shared<CollisionComponent>();
    auto physics = std::make_shared<PhysicsComponent>(collision);

    BoundingBox box = { .min = Vector3{-1, -1, -1}, .max = Vector3{1, 1, 1} };
    collision->SetBoundingBox(box);

    for (auto _ : state) {
        physics->Update(1.0f/60.0f);
    }
}

BENCHMARK(BM_PhysicsComponentUpdate);

static void BM_CollisionSystemCheckCollision(benchmark::State& state) {
    auto collisionSystem = std::make_shared<CollisionSystem>();

    // Add test objects
    for (int i = 0; i < 100; i++) {
        auto component = std::make_shared<CollisionComponent>();
        BoundingBox box = {
            .min = Vector3{static_cast<float>(i), 0, 0},
            .max = Vector3{static_cast<float>(i + 1), 1, 1}
        };
        component->SetBoundingBox(box);
        collisionSystem->AddCollisionComponent(component);
    }

    for (auto _ : state) {
        auto components = collisionSystem->GetCollisionComponents();
        if (components.size() >= 2) {
            collisionSystem->CheckCollision(*components[0], *components[1]);
        }
    }
}

BENCHMARK(BM_CollisionSystemCheckCollision);

static void BM_CollisionSystemBuildBVH(benchmark::State& state) {
    auto collisionSystem = std::make_shared<CollisionSystem>();

    // Add many objects
    for (int i = 0; i < state.range(0); i++) {
        auto component = std::make_shared<CollisionComponent>();
        BoundingBox box = {
            .min = Vector3{static_cast<float>(i), 0, 0},
            .max = Vector3{static_cast<float>(i + 1), 1, 1}
        };
        component->SetBoundingBox(box);
        collisionSystem->AddCollisionComponent(component);
    }

    for (auto _ : state) {
        collisionSystem->BuildBVH();
    }
}

BENCHMARK(BM_CollisionSystemBuildBVH)->Range(8, 8192);

static void BM_MapGenerationTest(benchmark::State& state) {
    auto generator = std::make_shared<ParkourMapGenerator>();

    for (auto _ : state) {
        generator->SetDifficulty(DifficultyLevel::Test);
        generator->GenerateMap();
    }
}

BENCHMARK(BM_MapGenerationTest);

static void BM_MapGenerationEasy(benchmark::State& state) {
    auto generator = std::make_shared<ParkourMapGenerator>();

    for (auto _ : state) {
        generator->SetDifficulty(DifficultyLevel::Easy);
        generator->GenerateMap();
    }
}

BENCHMARK(BM_MapGenerationEasy);

static void BM_MapGenerationMedium(benchmark::State& state) {
    auto generator = std::make_shared<ParkourMapGenerator>();

    for (auto _ : state) {
        generator->SetDifficulty(DifficultyLevel::Medium);
        generator->GenerateMap();
    }
}

BENCHMARK(BM_MapGenerationMedium);

static void BM_MapGenerationHard(benchmark::State& state) {
    auto generator = std::make_shared<ParkourMapGenerator>();

    for (auto _ : state) {
        generator->SetDifficulty(DifficultyLevel::Hard);
        generator->GenerateMap();
    }
}

BENCHMARK(BM_MapGenerationHard);

static void BM_GameInitialization(benchmark::State& state) {
    for (auto _ : state) {
        auto game = std::make_shared<Game>();
        game->Initialize();
    }
}

BENCHMARK(BM_GameInitialization);

static void BM_GameUpdate(benchmark::State& state) {
    auto game = std::make_shared<Game>();
    game->Initialize();

    for (auto _ : state) {
        game->Update(1.0f/60.0f);
    }
}

BENCHMARK(BM_GameUpdate);

static void BM_Vector3Operations(benchmark::State& state) {
    std::vector<Vector3> vectors;
    for (int i = 0; i < 1000; i++) {
        vectors.push_back(Vector3{static_cast<float>(i), static_cast<float>(i), static_cast<float>(i)});
    }

    for (auto _ : state) {
        Vector3 result{0, 0, 0};
        for (const auto& vec : vectors) {
            result = result + vec;
        }
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_Vector3Operations);

static void BM_CollisionDetectionManyObjects(benchmark::State& state) {
    auto collisionSystem = std::make_shared<CollisionSystem>();

    // Add many collision objects
    for (int i = 0; i < state.range(0); i++) {
        auto component = std::make_shared<CollisionComponent>();
        BoundingBox box = {
            .min = Vector3{static_cast<float>(i * 2), 0, 0},
            .max = Vector3{static_cast<float>(i * 2 + 1), 1, 1}
        };
        component->SetBoundingBox(box);
        collisionSystem->AddCollisionComponent(component);
    }

    collisionSystem->BuildBVH();

    for (auto _ : state) {
        auto components = collisionSystem->GetCollisionComponents();
        if (!components.empty()) {
            // Test collision with first component
            for (size_t i = 1; i < components.size(); i++) {
                collisionSystem->CheckCollision(*components[0], *components[i]);
            }
        }
    }
}

BENCHMARK(BM_CollisionDetectionManyObjects)->Range(10, 1000);

static void BM_MemoryAllocation(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::shared_ptr<CollisionComponent>> components;

        for (int i = 0; i < 1000; i++) {
            auto component = std::make_shared<CollisionComponent>();
            BoundingBox box = {
                .min = Vector3{static_cast<float>(i), 0, 0},
                .max = Vector3{static_cast<float>(i + 1), 1, 1}
            };
            component->SetBoundingBox(box);
            components.push_back(component);
        }

        benchmark::DoNotOptimize(components);
    }
}

BENCHMARK(BM_MemoryAllocation);

static void BM_StringOperations(benchmark::State& state) {
    std::vector<std::string> strings;
    for (int i = 0; i < 1000; i++) {
        strings.push_back("Component_" + std::to_string(i));
    }

    for (auto _ : state) {
        std::string result;
        for (const auto& str : strings) {
            result += str + "_";
        }
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_StringOperations);

static void BM_JSONParsing(benchmark::State& state) {
    std::string json = R"({
        "platforms": [
            {"position": [0, 0, 0], "size": [1, 1, 1]},
            {"position": [1, 0, 0], "size": [1, 1, 1]},
            {"position": [2, 0, 0], "size": [1, 1, 1]}
        ]
    })";

    for (auto _ : state) {
        nlohmann::json parsed = nlohmann::json::parse(json);
        benchmark::DoNotOptimize(parsed);
    }
}

BENCHMARK(BM_JSONParsing);

static void BM_RenderingOverhead(benchmark::State& state) {
    auto game = std::make_shared<Game>();
    game->Initialize();

    for (auto _ : state) {
        game->Render();
    }
}

BENCHMARK(BM_RenderingOverhead);

static void BM_PhysicsSimulation(benchmark::State& state) {
    std::vector<std::shared_ptr<PhysicsComponent>> physicsObjects;

    for (int i = 0; i < 100; i++) {
        auto collision = std::make_shared<CollisionComponent>();
        auto physics = std::make_shared<PhysicsComponent>(collision);

        BoundingBox box = {
            .min = Vector3{static_cast<float>(i), 0, 0},
            .max = Vector3{static_cast<float>(i + 1), 1, 1}
        };
        collision->SetBoundingBox(box);

        physics->SetVelocity(Vector3{1, 0, 0});
        physicsObjects.push_back(physics);
    }

    for (auto _ : state) {
        for (auto& physics : physicsObjects) {
            physics->Update(1.0f/60.0f);
        }
    }
}

BENCHMARK(BM_PhysicsSimulation);

static void BM_ComplexSceneUpdate(benchmark::State& state) {
    auto game = std::make_shared<Game>();
    game->Initialize();

    // Generate complex map
    auto mapGenerator = std::make_shared<ParkourMapGenerator>();
    mapGenerator->SetDifficulty(DifficultyLevel::Hard);
    mapGenerator->GenerateMap();

    auto map = mapGenerator->GetGeneratedMap();
    game->LoadMap(map);

    for (auto _ : state) {
        game->Update(1.0f/60.0f);
    }
}

BENCHMARK(BM_ComplexSceneUpdate);

static void BM_ResourceLoading(benchmark::State& state) {
    for (auto _ : state) {
        auto mapLoader = std::make_shared<MapLoader>();

        // Test loading a map multiple times
        for (int i = 0; i < 10; i++) {
            auto map = mapLoader->LoadMap("test_map.json");
            if (map) {
                benchmark::DoNotOptimize(map);
            }
        }
    }
}

BENCHMARK(BM_ResourceLoading);

static void BM_MathOperations(benchmark::State& state) {
    for (auto _ : state) {
        Vector3 result{0, 0, 0};

        for (int i = 0; i < 10000; i++) {
            Vector3 vec{static_cast<float>(i), static_cast<float>(i * 2), static_cast<float>(i * 3)};
            result = result + vec * 0.1f;
        }

        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_MathOperations);

static void BM_ContainerOperations(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<Vector3> vectors;

        // Fill vector
        for (int i = 0; i < 1000; i++) {
            vectors.push_back(Vector3{static_cast<float>(i), 0, 0});
        }

        // Process vector
        float sum = 0;
        for (const auto& vec : vectors) {
            sum += vec.x + vec.y + vec.z;
        }

        benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(BM_ContainerOperations);

static void BM_MapComplexityScaling(benchmark::State& state) {
    auto generator = std::make_shared<ParkourMapGenerator>();

    for (auto _ : state) {
        generator->SetDifficulty(static_cast<DifficultyLevel>(state.range(0)));
        generator->GenerateMap();
    }
}

BENCHMARK(BM_MapComplexityScaling)->DenseRange(0, 4);

static void BM_CollisionStressTest(benchmark::State& state) {
    auto collisionSystem = std::make_shared<CollisionSystem>();

    // Add many overlapping objects
    for (int i = 0; i < state.range(0); i++) {
        auto component = std::make_shared<CollisionComponent>();
        BoundingBox box = {
            .min = Vector3{0, 0, 0},
            .max = Vector3{1, 1, 1}
        };
        component->SetBoundingBox(box);
        collisionSystem->AddCollisionComponent(component);
    }

    collisionSystem->BuildBVH();

    for (auto _ : state) {
        auto components = collisionSystem->GetCollisionComponents();
        for (size_t i = 0; i < components.size(); i++) {
            for (size_t j = i + 1; j < components.size(); j++) {
                collisionSystem->CheckCollision(*components[i], *components[j]);
            }
        }
    }
}

BENCHMARK(BM_CollisionStressTest)->Range(5, 50);

BENCHMARK_MAIN();