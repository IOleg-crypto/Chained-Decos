//
// Created by I#Oleg
//
#include "Window.h"

Window::Window() : m_screenX(0), m_screenY(0) {}

Window::Window(const int screenX, const int screenY, const std::string &windowName)
    : m_screenX(screenX), m_screenY(screenY), m_WindowName(windowName), m_player() {}

Window::~Window() {
    CloseWindow();
}
void Window::Init() {
    InitWindow(m_screenX, m_screenY, m_WindowName.c_str());
    SetTargetFPS(60);

    m_models.AddModel("Resources/plane.obj");
}

void Window::Run() {
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(m_player.getCamera());

        m_models.DrawAll(5 ,4 ,4);
        DrawGrid(10, 1.0f); // Draw a grid for reference
        EndMode3D();
        DrawText("Welcome to Raylib!", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }
}
