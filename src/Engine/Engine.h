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
                                  Vector3 position, float scale);

    // Helper methods for collision creation
    std::shared_ptr<Collision> CreateBaseCollision(const Model &model, const std::string &modelName,
                                                   const ModelFileConfig *config,
                                                   bool needsPreciseCollision);

    Collision CreatePreciseInstanceCollision(const Model &model, Vector3 position, float scale,
                                             const ModelFileConfig *config);

    Collision CreateSimpleInstanceCollision(const Collision &cachedCollision, Vector3 position,
                                            float scale);

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
    bool HasExtremeVelocity(const Vector3 &vel) const;
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

    // Collision cache to prevent rebuilding octrees for same models
    std::unordered_map<std::string, std::shared_ptr<Collision>> m_collisionCache;

    // Counter for precise collisions per model to limit memory usage
    std::unordered_map<std::string, int> m_preciseCollisionCount;
    static constexpr int MAX_PRECISE_COLLISIONS_PER_MODEL = 50; // Limit precise collisions

    // Helper function to create cache key for scaled models
    std::string MakeCollisionCacheKey(const std::string &modelName, float scale) const;

    // Game State
    Menu m_menu;
    bool m_showMenu;
    bool m_shouldExit;
    bool m_windowInitialized; // Track if this Engine instance initialized the window

    // Debug State
    bool m_showDebug;
    bool m_showCollisionDebug;
    bool m_showOctreeDebug;
};

#endif // ENGINE_H