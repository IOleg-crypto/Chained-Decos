//
// Created by I#Oleg
//
#include "Window.h"
#include "rcamera.h"
#include "rlImGui.h"

#include <utility>

Window::Window(const int screenX, const int screenY, std::string windowName)
    : m_screenX(screenX), m_screenY(screenY), m_WindowName(std::move(windowName)) , m_showDebug(false) {

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
    rlImGuiShutdown();		// cleans up ImGui
    CloseWindow();
}
void Window::Init() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_FULLSCREEN_MODE);
    InitWindow(m_screenX, m_screenY, m_WindowName.c_str());
    HideCursor();
    SetTargetFPS(60);
    rlImGuiSetup(true);
    m_models.LoadModelsFromJson(std::string(GetWorkingDirectory()) + "\\src\\models.json");
}
void Window::Run() {
    while (!WindowShouldClose()) {
        Update();
        Render();
    }
}

void Window::Update() {
    KeyboardShortcut();
    if (m_showDebug) {
        DrawDebugInfo(m_player.getCamera(), m_player.GetCameraMode());
    }
}

void Window::Render() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode3D(m_player.getCamera());
    DrawScene3D();
    EndMode3D();


    EndDrawing();
}
void Window::DrawScene3D() {
    DrawGrid(50, 5.0f);
    DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 500.0f, 500.0f }, LIGHTGRAY); // Draw ground
    m_models.DrawAllModels();
}

void Window::DrawDebugInfo(const Camera &camera , const int &cameraMode) {
    // before your game loop
   	// sets up ImGui with ether a dark or light default theme

    // inside your game loop, between BeginDrawing() and EndDrawing()
    rlImGuiBegin();			// starts the ImGui content mode. Make all ImGui calls after this

    rlImGuiEnd();			// ends the ImGui content mode. Make all ImGui calls before this

    // after your game loop is over, before you close the window
}

void Window::KeyboardShortcut() {
    if (IsKeyPressed(KEY_F5)) {
        ToggleFullscreen();
    }
    // Update only camera rotation (not movement) using raylib
    m_player.Update();

    Camera &camera = m_player.getCamera();
    int &cameraMode = m_player.GetCameraMode();

    if (IsKeyPressed(KEY_ONE))
    {
        cameraMode = CAMERA_FREE;
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // Reset roll
    }

    if (IsKeyPressed(KEY_TWO))
    {
        cameraMode = CAMERA_FIRST_PERSON;
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // Reset roll
    }

    if (IsKeyPressed(KEY_THREE))
    {
        cameraMode = CAMERA_THIRD_PERSON;
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // Reset roll
    }

    if (IsKeyPressed(KEY_FOUR))
    {
        cameraMode = CAMERA_ORBITAL;
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // Reset roll
    }

    // Switch camera projection
    if (IsKeyPressed(KEY_P))
    {
        if (camera.projection == CAMERA_PERSPECTIVE)
        {
            // Create isometric view
            cameraMode = CAMERA_THIRD_PERSON;
            // Note: The target distance is related to the render distance in the orthographic projection
            camera.position = (Vector3){ 0.0f, 2.0f, -100.0f };
            camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
            camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
            camera.projection = CAMERA_ORTHOGRAPHIC;
            camera.fovy = 20.0f; // near plane width in CAMERA_ORTHOGRAPHIC
            CameraYaw(&camera, -135 * DEG2RAD, true);
            CameraPitch(&camera, -45 * DEG2RAD, true, true, false);
        }
        else if (camera.projection == CAMERA_ORTHOGRAPHIC)
        {
            // Reset to default view
            cameraMode = CAMERA_THIRD_PERSON;
            camera.position = (Vector3){ 0.0f, 2.0f, 10.0f };
            camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
            camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
            camera.projection = CAMERA_PERSPECTIVE;
            camera.fovy = 60.0f;
        }
    }


    // Toggle debug info with F5
    if (IsKeyPressed(KEY_FIVE)) {
        m_showDebug = !m_showDebug;
    }


}

