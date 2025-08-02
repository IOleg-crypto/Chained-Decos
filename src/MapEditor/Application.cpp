//
// Created by I#Oleg.
//
#include <MapEditor/Application.h>

Application::Application(int width, int height)
    : m_width(width), m_height(height) , m_WindowName("ChainedEditor") , m_editor(std::make_unique<Editor>()) {}

Application::~Application() {
    // Cleanup window resources
    CloseWindow();
}

void Application::Init() const {
    // Configure window settings
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_width, m_height, m_WindowName.c_str());
    SetTargetFPS(60);
    
    // Initialize ImGui for the editor interface
    ImGui::CreateContext();
    ImGui_ImplRaylib_Init();
}

void Application::Run() const {
    // Main application loop
    while(!WindowShouldClose())
    {
        // Update editor state
        m_editor->Update();
        
        // Begin rendering frame
        BeginDrawing();
        ClearBackground(DARKGRAY);
        
        // Render 3D scene
        BeginMode3D(m_editor->GetCameraController()->getCamera());
        m_editor->GetCameraController()->SetCameraMode(CAMERA_FREE);
        DrawGrid(50, 1.0f);  // Draw reference grid
        
        // Render all editor objects
        m_editor->Render();
        
        EndMode3D();
        
        // Render ImGui interface on top
        m_editor->RenderImGui();
        
        EndDrawing(); 
    }
}
