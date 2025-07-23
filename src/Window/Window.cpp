//
// Created by I#Oleg on 20.07.2025.
//

#include "Window.h"

Window::Window() : m_screenX(0) , m_screenY(0){

}

Window::Window(const int screenX, const int screenY, const std::string &windowName) : m_screenX(screenX) , m_screenY(screenY) , m_WindowName(windowName) , m_player() {
}
// Closes window
Window::~Window() {
    CloseWindow();
}
// Set default settings for raylib window(game loop)
void Window::Init() const {
    InitWindow(m_screenX , m_screenY , m_WindowName.c_str());
    SetTargetFPS(60);
}

void Window::Run() const {
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(m_player.getCamera());
        if (IsKeyPressed('Z')) m_player.getCamera().target = (Vector3){ 0.0f, 0.0f, 0.0f };
        DrawCube((Vector3){ 0.0f, 1.0f, 0.0f }, 2.0f, 2.0f, 2.0f, BLUE);         // Куб
        DrawCubeWires((Vector3){ 0.0f, 1.0f, 0.0f }, 2.0f, 2.0f, 2.0f, MAROON); // Контури куба
        EndMode3D();
        EndDrawing();
    }
}
