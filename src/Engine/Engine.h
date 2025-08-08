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

// # ------------------------------------------------------
// # Engine - main class , that creates window using raylib
// # ------------------------------------------------------
class Engine
{
  private:
    // Screen resolution
    int m_screenX{};
    int m_screenY{};

    // Window title
    std::string m_windowName;

    // Game objects
    Player m_player;
    Models m_models;
    InputManager m_manager;

    // Debug flag
    bool m_showDebug{false};
    // Collision debug visualization
    bool m_showCollisionDebug{false};
    // Menu
    Menu m_menu;
    // Show menu
    bool m_showMenu = true;
    // For exit
    bool m_shouldExit{};

    // Model for player
    Model m_playerModelMesh;
    CollisionManager m_collisionManager;
    Collision m_collision;
    Collision m_CubeCollision;
    // Player model usage flag
    bool m_usePlayerModel = true;

  public:
    // Constructors & Destructor
    Engine() = default;
    Engine(int screenX, int screenY);
    ~Engine();
    // Deleted copy/move constructors (don`t needed)
    Engine(const Engine &other) = delete;
    Engine(Engine &&other) = delete;

  public:
    // Initialization
    void Init();      // Initializes the engine and window
    void InitInput(); // Initializes input handling

    // Main loop
    void Run();

    // Input
    void HandleKeyboardShortcuts() const;
    void Update(); // Updates input and player logic

    // Rendering
    void Render(); // Renders all objects and UI
    void LoadPlayerModel();

    void DrawScene3D(); // Renders 3D scene

    // For ImGui(Load better font)
    void InitImGuiFont() const;
    // Debug (using ImGui)
    void DrawDebugInfo(const Camera &camera, const int &cameraMode);

    // Menu
    void DrawMenu();
    void ToggleMenu() { m_showMenu = !m_showMenu; }
    void InitCollisions();
};

#endif // ENGINE_H
