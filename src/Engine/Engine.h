//
// Engine.h - Main Engine Class
// Created by I#Oleg on 20.07.2025.
//
#ifndef ENGINE_H
#define ENGINE_H

// Standard library
#include <memory>
#include <string>
#include <unordered_map>
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
    Engine();
    Engine(int screenX, int screenY);
    ~Engine();
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

private:
    void Update();
    void UpdatePlayer();
    void HandlePlayerCollision();
    void UpdatePhysics();
    void CheckPlayerBounds();
    void HandleKeyboardShortcuts() const;
    void Render();
    void TestOctreeRayCasting();
    void OptimizeModelPerformance();
    void TracePlayerIssue(const Vector3 &pos, const Vector3 &vel) const;
    bool IsPlayerOutOfBounds(const Vector3 &pos) const;
    void EnsureGroundPlaneExists();

public:
    void RequestExit();
    bool IsRunning() const;

private:
    // Window & Display
    int m_screenX;
    int m_screenY;
    std::string m_windowName;

    // Core Game Systems
    Player m_player;
    Models m_models;
    InputManager m_manager;
    PhysicsComponent m_physics;
    CollisionManager m_collisionManager; // Legacy collision system
    RenderManager m_renderManager;

    // Game State
    Menu m_menu;
    bool m_showMenu;
    bool m_shouldExit;
    bool m_windowInitialized; // Track if this Engine instance initialized the window

    // Debug State
    bool m_showDebug;
    bool m_showCollisionDebug;
    // For engine init
    bool m_isEngineInit;
};

#endif // ENGINE_H