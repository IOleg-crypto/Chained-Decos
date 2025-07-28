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
    HideCursor();
    SetTargetFPS(60);

    m_models.LoadModelsFromJson(std::string(GetWorkingDirectory()) + "\\src\\models.json");
}
void Window::Run() {
    while (!WindowShouldClose()) {
        Update();
        Render();
    }
}

void Window::Update() {
    m_models.CheckCollision(m_player);
    KeyboardShortcut();
}

void Window::Render() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(m_player.getCamera());
    DrawScene3D();
    EndMode3D();

    DrawOverlay2D();

    EndDrawing();
}
void Window::DrawScene3D() {
    DrawGrid(50, 5.0f);

    m_models.DrawAllModels();

}

void Window::DrawOverlay2D() {
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, DARKGRAY);
}

void Window::KeyboardShortcut() {
    if (IsKeyPressed(KEY_F5)) {
        ToggleFullscreen();
    }

    // Update only camera rotation (not movement) using raylib
    //m_player.Update();

    // Move logic (WASD)
    Vector3 forward = Vector3Normalize(Vector3Subtract(
        m_player.getCamera().target,
        m_player.getCamera().position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, m_player.getCamera().up));

    if (IsKeyDown(KEY_LEFT_SHIFT)) m_player.SetSpeed(0.5f);
    else m_player.SetSpeed(0.1f);

    float speed = m_player.GetSpeed();

    if (IsKeyDown(KEY_W)) {
        m_player.Move(Vector3Scale(forward, speed));
    }
    if (IsKeyDown(KEY_S)) {
        m_player.Move(Vector3Scale(forward, -speed));
    }
    if (IsKeyDown(KEY_D)) {
        m_player.Move(Vector3Scale(right, speed));
    }
    if (IsKeyDown(KEY_A)) {
        m_player.Move(Vector3Scale(right, -speed));
    }


}

