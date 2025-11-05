#include "EditorApplication.h"
#include "Editor/Editor.h"
#include "Engine/CameraController/CameraController.h"
#include "Engine/Model/Model.h"
#include "Modules/EditorModule.h"
#include "Engine/Kernel/KernelServices.h"
#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>

EditorApplication::EditorApplication()
    : EngineApplication()
{
    auto& config = GetConfig();
    config.windowName = "ChainedEditor";
    config.width = 1280;
    config.height = 720;
    config.resizable = true;
    config.enableMSAA = true;
}

EditorApplication::~EditorApplication()
{
    TraceLog(LOG_INFO, "[EditorApplication] Destructor called.");
}

void EditorApplication::OnPreInitialize()
{
    TraceLog(LOG_INFO, "[EditorApplication] Pre-initialization...");
}

void EditorApplication::OnInitializeServices()
{
    TraceLog(LOG_INFO, "[EditorApplication] Initializing editor components...");

    // Create Editor components using engine services
    auto camera = std::make_shared<CameraController>();
    auto modelLoader = std::make_unique<ModelLoader>();
    m_editor = std::make_unique<Editor>(camera, std::move(modelLoader));
    
    TraceLog(LOG_INFO, "[EditorApplication] Editor components initialized.");
}

void EditorApplication::OnRegisterProjectModules()
{
    TraceLog(LOG_INFO, "[EditorApplication] Registering editor modules...");
    
    if (auto engine = GetEngine()) {
        engine->RegisterModule(std::make_unique<EditorModule>());
        TraceLog(LOG_INFO, "[EditorApplication] Editor modules registered.");
    } else {
        TraceLog(LOG_ERROR, "[EditorApplication] Engine not available!");
    }
}

void EditorApplication::OnRegisterProjectServices()
{
    TraceLog(LOG_INFO, "[EditorApplication] Registering editor services...");
    // Editor doesn't register additional services yet
    // Can add EditorService if needed
}

void EditorApplication::OnPostInitialize()
{
    TraceLog(LOG_INFO, "[EditorApplication] Post-initialization...");
    
    // Configure ImGui for Editor (custom settings)
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    
    // Set up custom font
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/Lato/Lato-Black.ttf", 16.0f);
    // Don't build fonts here - will be built in RenderManager::BeginFrame()
    
    // Preload models after window initialization
    if (m_editor) {
        m_editor->PreloadModelsFromResources();
        m_editor->LoadSpawnTexture();
    }
    
    // Set window icon
    Image icon = LoadImage(PROJECT_ROOT_DIR "/resources/icons/ChainedDecosMapEditor.jpg");
    if (icon.data != nullptr) {
        ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        SetWindowIcon(icon);
        UnloadImage(icon);
    }
    
    TraceLog(LOG_INFO, "[EditorApplication] Post-initialization complete.");
}

void EditorApplication::OnPostUpdate(float deltaTime)
{
    (void)deltaTime;
    
    if (!m_editor) return;
    
    // Update editor state
    m_editor->Update();
    
    // Handle editor input
    m_editor->HandleInput();
}

void EditorApplication::OnPostRender()
{
    if (!m_editor) return;
    
    // Editor rendering
    // BeginFrame() already called in Engine::Render() via RenderManager::BeginFrame()
    // EndFrame() will be called in Engine::Render() via RenderManager::EndFrame()
    
    // Clear background before 3D scene
    ClearBackground(DARKGRAY);
    
    // Render 3D scene for Editor
    auto cameraController = m_editor->GetCameraController();
    if (cameraController) {
        BeginMode3D(cameraController->GetCamera());
        cameraController->SetCameraMode(CAMERA_FREE);
        
        // Draw grid
        DrawGrid(m_editor->GetGridSize(), 1.0f);
        
        // Render all editor objects
        m_editor->Render();
        
        EndMode3D();
    }
    
    // Begin ImGui frame for Editor UI
    // Note: rlImGuiBegin() must be called here, before rendering ImGui
    // rlImGuiEnd() will be called AFTER EndFrame() but before kernel->Render()
    rlImGuiBegin();
    
    // Render ImGui interface
    m_editor->RenderImGui();
    
    // rlImGuiEnd() will be called after EndFrame() - but we need to call it here
    // because kernel->Render() is called after EndFrame() and may conflict
    // Actually, we should keep it here and let kernel->Render() handle its own ImGui if needed
    rlImGuiEnd();
}

void EditorApplication::OnPreShutdown()
{
    TraceLog(LOG_INFO, "[EditorApplication] Pre-shutdown...");
    // Editor cleans up its own resources in destructor
}

