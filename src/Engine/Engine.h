//
// Created by I#Oleg on 20.07.2025.
//

#ifndef ENGINE_H
#define ENGINE_H

// Standard library
#include <string>

// Raylib
#include <raylib.h>

// Project headers
#include <Player/Player.h>
#include <Model/Model.h>
#include <Menu/Menu.h>
#include <Input/InputManager.h>
// # ------------------------------------------------------
// # Engine - main class , that creates window using raylib
// # ------------------------------------------------------
class Engine {
private:
    // Screen resolution
    int m_screenX{};
    int m_screenY{};

    // Window title
    std::string m_WindowName;

    // Game objects
    Player m_player;
    Models m_models;
    InputManager manager;

    // Debug flag
    bool m_showDebug{false};
    // Menu
    Menu m_menu;
    // Show menu
    bool showMenu = true;
    // For exit
    bool m_shouldExit{};
public:
    // Constructors & Destructor
    Engine() = default;
    Engine(int screenX, int screenY);
    ~Engine();

    // Deleted copy/move constructors (not needed)
    Engine(const Engine& other) = delete;
    Engine(Engine&& other) = delete;

    // Initialization
    void Init() const;

    // Main loop
    void Run();

    // Input
    void KeyboardShortcut() const;
    void Update(); // Updates input and player logic

    // Rendering
    void Render();       // Renders all objects and UI
    void LoadPlayerModel();

    void DrawScene3D();  // Renders 3D scene

    // For ImGui(Load better font)
    void InitImGuiFont() const;
    // Debug (using ImGui)
    void DrawDebugInfo(const Camera &camera , const int &cameraMode);
    void InitInput();

};

#endif // ENGINE_H
