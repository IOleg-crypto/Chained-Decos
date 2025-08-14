//
// Engine.h - Main Engine Class
// Created by I#Oleg on 20.07.2025.
//
#ifndef ENGINE_H
#define ENGINE_H

// Standard library
#include <memory>
#include <string>
#include <vector>

// Raylib
#include <raylib.h>

// Project headers
#include <Collision/CollisionDebugRenderer.h>
#include <Collision/CollisionManager.h>
#include <Input/InputManager.h>
#include <Menu/Menu.h>
#include <Model/Model.h>
#include <Player/Player.h>
#include <Render/RenderManager.h>
#include <World/Physics.h>

/**
 * Main Engine class - manages the entire game application
 *
 * Responsibilities:
 *  - Window creation and management (using raylib)
 *  - Game loop (update/render cycle)
 *  - Managing core game objects (player, models, input, physics)
 *  - Debug information and collision visualization
 *  - Menu system integration

 */
class Engine
{
public:
    // ==================== CONSTRUCTORS & DESTRUCTOR ====================
    Engine();
    Engine(int screenX, int screenY);
    ~Engine();

    // Non-copyable and non-movable
    Engine(const Engine &other) = delete;
    Engine(Engine &&other) = delete;
    Engine &operator=(const Engine &other) = delete;
    Engine &operator=(Engine &&other) = delete;

    // ==================== MAIN API ====================
    void Init();
    void Run();
    void ToggleMenu();

private:
    void InitInput();
    void InitCollisions();
    void CreateAutoCollisionsFromModels(); // Automatically create collisions for all models with
                                           // hasCollision=true
    bool CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                  Vector3 position,
                                  float scale); // Create BVH from model geometry
    void Update();
    void UpdatePlayer();
    void UpdatePhysics();
    void CheckPlayerBounds();
    void HandleKeyboardShortcuts() const;
    void HandleMousePicking();
    void Render();
    void TestOctreeRayCasting();
    void TestMouseRayCasting();
    void OptimizeModelPerformance();
    // Future features (currently unused)
    void LoadAdditionalModels();
    void SpawnPickups();

public:
    void RequestExit();
    bool IsRunning() const;

private:
    // Window & Display
    int m_screenX{};
    int m_screenY{};
    std::string m_windowName;

    // Core Game Systems
    Player m_player;
    Models m_models;
    InputManager m_manager;
    PhysicsComponent m_physics;
    CollisionManager m_collisionManager;
    RenderManager m_renderManager;

    // Game State
    Menu m_menu;
    bool m_showMenu = true;
    bool m_shouldExit{false};
    bool m_usePlayerModel = true;

    // Debug State
    bool m_showDebug{false};
    bool m_showCollisionDebug{false};

    // Legacy (consider removing)
    Collision m_collision{};
};

#endif // ENGINE_H
