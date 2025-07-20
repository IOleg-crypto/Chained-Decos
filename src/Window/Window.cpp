//
// Created by I#Oleg on 20.07.2025.
//

#include "Window.h"

Window::Window() : m_screenX(0) , m_screenY(0) , m_WindowName("") {

}

Window::Window(const int screenX, const int screenY, const std::string &windowName) : m_screenX(screenX) , m_screenY(screenY) , m_WindowName(windowName) {
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
        DrawText("Hello i set up raylib", 20, 20, 20, BLACK);
        EndDrawing();
    }
}
