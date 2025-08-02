//
// Created by I#Oleg.
//
#include <MapEditor/Application.h>

Application::Application(const int width, const int height)
    : m_width(width), m_height(height) , m_WindowName("ChainedEditor") , m_editor(std::make_unique<Editor>()) {}

Application::~Application() {
    // Cleanup ImGui first, then window
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
    
    // Configure ImGui settings for better interaction
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard navigation
    
    // Set up custom font
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/Lato/Lato-Black.ttf", 16.0f);
    io.Fonts->Build();
    
    // Set up ImGui style for better visibility
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Configure style for better window interaction
    style.WindowPadding = ImVec2(8, 8);
    style.FramePadding = ImVec2(4, 4);
    style.ItemSpacing = ImVec2(8, 4);
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 8.0f;
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    
    // Colors for better visibility
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.26f, 0.26f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
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
