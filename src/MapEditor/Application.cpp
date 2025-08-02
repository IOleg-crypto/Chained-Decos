//
// Created by I#Oleg.
//
#include <MapEditor/Application.h>

Application::Application(const int width, const int height)
    : m_width(width), m_height(height) , m_WindowName("ChainedEditor") , m_editor(std::make_unique<Editor>()) {}

Application::~Application() {
    // Cleanup window resources
    rlImGuiShutdown();
    CloseWindow();
}

void Application::Init() const {
    // Configure window settings
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_width, m_height, m_WindowName.c_str());
    SetTargetFPS(60);
    
    // Check if window was created successfully
    if (!IsWindowReady()) {
        std::cerr << "Failed to create window!" << std::endl;
        return;
    }
    
    // Initialize ImGui AFTER window is created
    rlImGuiSetup(true);

    // Render all ImGui panels
    const ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/Lato/Lato-Black.ttf", 16.0f);
    io.Fonts->Build();
}

void Application::Run() const {
    // Check if window is ready before starting the loop
    if (!IsWindowReady()) {
        std::cerr << "Window not ready, cannot start application loop!" << std::endl;
        return;
    }
    
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
