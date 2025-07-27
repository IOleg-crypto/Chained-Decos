//
// Created by I#Oleg
//
#include "Window.h"

#include <utility>

Window::Window() : m_screenX(0), m_screenY(0) {}

Window::Window(const int screenX, const int screenY, std::string windowName)
    : m_screenX(screenX), m_screenY(screenY), m_WindowName(std::move(windowName)), m_player() {

    if (m_screenX <= 0 || m_screenY <= 0) {
        TraceLog(LOG_WARNING, "[Screen] Invalid screen size: %d x %d. Setting default size 800x600.", m_screenX, m_screenY);
        m_screenX = 800;
        m_screenY = 600;
    }

    if (m_WindowName.empty()) {
        TraceLog(LOG_WARNING, "Window name is empty. Setting default name: 'Raylib Window'.");
        m_WindowName = "Raylib Window";
    }
}

Window::~Window() {
    CloseWindow();
}
void Window::Init() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_FULLSCREEN_MODE); // Fullscreen + Resizable
    InitWindow(m_screenX, m_screenY, m_WindowName.c_str());
    SetTargetFPS(60);
    // Load model
    m_models.AddModel(GetWorkingDirectory() + std::string("/Resources/plane.glb"));
}

void Window::Run() {
    while (!WindowShouldClose()) {
        m_player.Update();
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(m_player.getCamera());
        m_models.DrawAll(0 ,0 ,0);
        DrawGrid(10, 1.0f); // Draw a grid for reference
        EndMode3D();
        GetFPS();
        DrawText("test", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }
}

void Window::KeyboardShortcut() const  {
    if (IsKeyPressed(KEY_F5)) {
        ToggleFullscreen();
    }
}
