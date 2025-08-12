//
// Created by I#Oleg on 20.07.2025.
//

#ifndef ENGINE_H
#define ENGINE_H

// Standard library
#include <string>
#include <vector>

// Raylib
#include <raylib.h>

// Project headers
#include <Collision/CollisionManager.h>
#include <Input/InputManager.h>
#include <Menu/Menu.h>
#include <Model/Model.h>
#include <Player/Player.h>
#include <World/Physics.h>

//
// Engine
// Main application class responsible for:
//  - Window creation and management (using raylib)
//  - Game loop and update/render calls
//  - Managing core game objects (player, models, input, physics)
//  - Debug and menu handling
//
class Engine
{
public:
    // Constructors & Destructor
    Engine() = default;
    Engine(int screenX, int screenY);
    ~Engine();

    // Deleted copy/move constructors - non-copyable
    Engine(const Engine &other) = delete;
    Engine(Engine &&other) = delete;

    // -------------------- Initialization --------------------

    // Initialize engine subsystems and window
    void Init();

    // Setup input bindings and handlers
    void InitInput();

    // -------------------- Main Loop --------------------

    // Start main application loop
    void Run();

    // Handle input shortcuts (e.g. toggle fullscreen)
    void HandleKeyboardShortcuts() const;

    // Update game state: input, physics, player, etc.
    void Update();

    // -------------------- Rendering --------------------

    // Render all game objects and UI
    void Render();

    // Load player model and prepare it for rendering
    void LoadPlayerModel();

    // Render the 3D scene objects
    void DrawScene3D();

    // -------------------- ImGui / Debug --------------------

    // Load better font for ImGui UI
    void InitImGuiFont() const;

    // Draw debug info overlay (using ImGui)
    void DrawDebugInfo(const Camera &camera, const int &cameraMode);

    // -------------------- Menu --------------------

    // Toggle menu visibility
    void ToggleMenu();

    // Initialize collision system
    void InitCollisions();

    // Private helper methods for better organization
    void UpdatePlayer();
    void UpdatePhysics();
    void CheckPlayerBounds();
    void RenderCollisionDebug();

    // Load additional models dynamically(Unused)
    void LoadAdditionalModels();
    void SpawnEnemies();
    void SpawnPickups();
    /////////////////////////////
    void OptimizeModelPerformance();

private:
    // Screen resolution
    int m_screenX{};
    int m_screenY{};

    // Window title
    std::string m_windowName;

    // Core game objects
    Player m_player;
    Models m_models;
    InputManager m_manager;
    PhysicsComponent m_physics;

    // Debug flags
    bool m_showDebug{false};
    bool m_showCollisionDebug{false};

    // Menu system
    Menu m_menu;
    bool m_showMenu = true;

    // Application state
    bool m_shouldExit{};

    // Collision management with separation of concerns
    CollisionManager m_collisionManager;
    class CollisionDebugRenderer *m_collisionDebugRenderer;
    Collision m_collision{};
    bool m_usePlayerModel = true;
};

#endif // ENGINE_H
