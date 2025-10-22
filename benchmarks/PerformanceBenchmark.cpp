#include <iostream>
#include <chrono>
#include <memory>
#include <vector>
#include <string>

#include "Engine/Physics/PhysicsComponent.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Collision/CollisionComponent.h"
#include "Game/Map/MapLoader.h"
#include "Game/Game.h"

// Simple timer class for benchmarking
class Timer {
public:
    void start() {
        m_start = std::chrono::high_resolution_clock::now();
    }

    void stop() {
        m_end = std::chrono::high_resolution_clock::now();
    }

    double elapsedMilliseconds() const {
        return std::chrono::duration<double, std::milli>(m_end - m_start).count();
    }

    double elapsedMicroseconds() const {
        return std::chrono::duration<double, std::micro>(m_end - m_start).count();
    }

private:
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_end;
};

// Simple benchmark function
template<typename Func>
double runBenchmark(const std::string& name, Func func, int iterations = 1000) {
    std::cout << "Running " << name << " (" << iterations << " iterations)..." << std::endl;

    // Warm up
    func();

    Timer timer;
    timer.start();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    timer.stop();

    double elapsed = timer.elapsedMilliseconds();
    double avgTime = elapsed / iterations;

    std::cout << "  Total time: " << elapsed << " ms" << std::endl;
    std::cout << "  Average time: " << avgTime << " ms per iteration" << std::endl;
    std::cout << "  Iterations per second: " << (1000.0 / avgTime) << std::endl;

    return avgTime;
}

int main() {
    std::cout << "Chained Decos - Performance Benchmarks" << std::endl;
    std::cout << "=====================================" << std::endl;

    // Physics Component Update Benchmark
    runBenchmark("Physics Component Update", []() {
        auto collision = std::make_shared<CollisionComponent>();
        auto physics = std::make_shared<PhysicsComponent>();

        BoundingBox box = { .min = Vector3{-1, -1, -1}, .max = Vector3{1, 1, 1} };
        collision->SetBoundingBox(box);

        physics->Update(1.0f/60.0f);
    }, 10000);

    // Collision System Benchmark
    runBenchmark("Collision System (100 objects)", []() {
        auto collisionManager = std::make_shared<CollisionManager>();

        // Add test objects
        for (int i = 0; i < 100; i++) {
            Collision collision(Vector3{static_cast<float>(i), 0, 0}, Vector3{1, 1, 1});
            collisionManager->AddCollider(std::move(collision));
        }

        // Test collision checking
        Collision testCollision(Vector3{50, 0, 0}, Vector3{1, 1, 1});
        [[maybe_unused]] bool hasCollision = collisionManager->CheckCollision(testCollision);
    }, 1000);

    // Vector3 Operations Benchmark
    runBenchmark("Vector3 Operations (1000 vectors)", []() {
        std::vector<Vector3> vectors;
        for (int i = 0; i < 1000; i++) {
            vectors.push_back(Vector3{static_cast<float>(i), static_cast<float>(i), static_cast<float>(i)});
        }

        Vector3 result{0, 0, 0};
        for (const auto& vec : vectors) {
            result = result + vec;
        }
    }, 1000);

    // Memory Allocation Benchmark
    runBenchmark("Memory Allocation (1000 objects)", []() {
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
    }, 100);

    // Map Loading Benchmark
    runBenchmark("Map Loading (All Maps)", []() {
        MapLoader loader;
        auto maps = loader.LoadAllMapsFromDirectory("resources/maps");
    }, 10);

    std::cout << std::endl;
    std::cout << "Benchmarks completed!" << std::endl;

    return 0;
}