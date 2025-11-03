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

    // Створюємо компоненти Editor
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
        TraceLog(LOG_WARNING, "[EditorApplication] No engine available, cannot register modules");
    }
}

void EditorApplication::OnRegisterProjectServices()
{
    TraceLog(LOG_INFO, "[EditorApplication] Registering editor services...");
    
    // Editor поки що не реєструє додаткові сервіси
    // Можна додати EditorService якщо потрібно
}

void EditorApplication::OnPostInitialize()
{
    TraceLog(LOG_INFO, "[EditorApplication] Post-initialization...");
    
    // Налаштування ImGui для Editor (кастомні налаштування)
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    
    // Set up custom font
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/Lato/Lato-Black.ttf", 16.0f);
    io.Fonts->Build();
    
    // Set up ImGui style for better visibility
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    
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
    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.95f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.26f, 0.26f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.26f, 0.26f, 0.67f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    
    // Preload моделі після ініціалізації вікна
    if (m_editor) {
        m_editor->PreloadModelsFromResources();
        m_editor->LoadSpawnTexture();
    }
    
    // Встановлюємо іконку вікна
    Image icon = LoadImage(PROJECT_ROOT_DIR "/resources/icons/ChainedDecosMapEditor.jpg");
    ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    SetWindowIcon(icon);
    UnloadImage(icon);
    
    TraceLog(LOG_INFO, "[EditorApplication] Editor application initialized.");
}

void EditorApplication::OnPostUpdate(float deltaTime)
{
    (void)deltaTime;  // Unused for now
    
    if (m_editor) {
        m_editor->Update();
        m_editor->HandleInput();
    }
}

void EditorApplication::OnPostRender()
{
    if (!m_editor) return;
    
    // Рендеринг Editor
    // BeginFrame() вже викликано в Engine::Render() через RenderManager::BeginFrame()
    // EndFrame() буде викликано в Engine::Render() через RenderManager::EndFrame()
    
    // Очищаємо фон перед 3D сценою
    ClearBackground(DARKGRAY);
    
    // Render 3D scene для Editor
    BeginMode3D(m_editor->GetCameraController()->GetCamera());
    m_editor->GetCameraController()->SetCameraMode(CAMERA_FREE);
    DrawGrid(m_editor->GetGridSize(), 1.0f);

    // Render all editor objects
    m_editor->Render();

    EndMode3D();

    // Begin ImGui frame для Editor UI
    // rlImGuiBegin() вже викликано в RenderManager, але можемо викликати знову
    // (він безпечно обробляє повторні виклики)
    rlImGuiBegin();
    
    // Render ImGui interface
    m_editor->RenderImGui();
    
    rlImGuiEnd();
}

void EditorApplication::OnPreShutdown()
{
    TraceLog(LOG_INFO, "[EditorApplication] Cleaning up editor resources...");
    
    // Editor сам очищає свої ресурси в деструкторі
    m_editor.reset();
    
    TraceLog(LOG_INFO, "[EditorApplication] Editor resources cleaned up.");
}

