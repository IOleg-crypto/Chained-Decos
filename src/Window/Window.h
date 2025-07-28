//
// Created by I#Oleg on 20.07.2025.
//

#ifndef WINDOW_H
#define WINDOW_H

// Standard library
#include <string>
#include <memory>

// Raylib
#include <raylib.h>

// Project headers
#include "../Player/Player.h"
#include "../Model/Model.h"

// # ------------------------------------------------------
// # Window - main class , that creates window using raylib
// # ------------------------------------------------------
class Window {
private:
    // Screen resolution
    int m_screenX{};
    int m_screenY{};

    // Window title
    std::string m_WindowName;

    // Game objects
    Player m_player;
    Models m_models;

    // Debug flag
    bool m_showDebug{false};

public:
    // Constructors & Destructor
    Window() = default;
    Window(int screenX, int screenY, std::string windowName);
    ~Window();

    // Deleted copy/move constructors (not needed)
    Window(const Window& other) = delete;
    Window(Window&& other) = delete;

    // Initialization
    void Init();

    // Main loop
    void Run();

    // Input
    void KeyboardShortcut();
    void Update(); // Updates input and player logic

    // Rendering
    void Render();       // Renders all objects and UI
    void DrawScene3D();  // Renders 3D scene

    // Debug
    static void DrawDebugInfo(const Camera& camera, const int& cameraMode);
};

#endif // WINDOW_H
