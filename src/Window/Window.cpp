//
// Created by I#Oleg
//
#include "Window.h"

#include <utility>

Window::Window(const int screenX, const int screenY, std::string windowName)
    : m_screenX(screenX), m_screenY(screenY), m_WindowName(std::move(windowName)), m_player() {

    if (m_screenX < 0 || m_screenY < 0) {
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
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_FULLSCREEN_MODE);
    InitWindow(m_screenX, m_screenY, m_WindowName.c_str());
    SetTargetFPS(60);

    m_models.LoadModelsFromJson(std::string(GetWorkingDirectory()) + "\\src\\models.json");
}
void Window::Run() {
    while (!WindowShouldClose()) {
        KeyboardShortcut();
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(m_player.getCamera());
        DrawGrid(20, 5.0f);
        m_models.DrawAllModels();
        EndMode3D();
        DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, DARKGRAY);

        EndDrawing();
    }
}

void Window::KeyboardShortcut()  {
    if (IsKeyPressed(KEY_F5)) {
        ToggleFullscreen();
    }
    // To avoid unnecessary movement
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        m_player.Update();
    }
}
