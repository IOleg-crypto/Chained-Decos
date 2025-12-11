#include "EditorApplication.h"
#include "Modules/EditorModule.h"
#include "core/engine/Engine.h"
#include "map_editor/Editor/Editor.h"
#include "scene/3d/camera/Core/CameraController.h"

#include "scene/resources/model/Core/Model.h"

//===============================================
#include <imgui.h>
#include <raylib.h>
#include <rlImGui/rlImGui.h>

EditorApplication::EditorApplication()
{
}

EditorApplication::~EditorApplication()
{
    TraceLog(LOG_INFO, "[EditorApplication] Destructor called.");
}

void EditorApplication::OnConfigure(EngineConfig &config)
{
    TraceLog(LOG_INFO, "[EditorApplication] Configuring application...");
    config.windowName = "Chained Decos - Map Editor";
    config.width = 1600;
    config.height = 900;
}

void EditorApplication::OnRegister()
{
    TraceLog(LOG_INFO, "[EditorApplication] Registering modules...");

    if (auto engine = GetEngine())
    {
        engine->RegisterModule(std::make_unique<EditorModule>());
        TraceLog(LOG_INFO, "[EditorApplication] Editor modules registered.");
    }
    else
    {
        TraceLog(LOG_ERROR, "[EditorApplication] Engine not available!");
    }
}

void EditorApplication::OnStart()
{
    TraceLog(LOG_INFO, "[EditorApplication] Starting application...");

    // Initialize Editor components
    auto camera = std::make_shared<CameraController>();
    auto modelLoader = std::make_unique<ModelLoader>();
    m_editor = std::make_unique<Editor>(camera, std::move(modelLoader));

    TraceLog(LOG_INFO, "[EditorApplication] Editor components initialized.");

    // Configure ImGui for Editor (custom settings)
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Set up custom font
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/Lato/Lato-Black.ttf", 16.0f);
    // Don't build fonts here - will be built in RenderManager::BeginFrame()

    // Preload models after window initialization
    if (m_editor)
    {
        m_editor->PreloadModelsFromResources();
        m_editor->LoadSpawnTexture();
    }

    // Set window icon
    Image icon = LoadImage(PROJECT_ROOT_DIR "/resources/icons/ChainedDecosMapEditor.jpg");
    if (icon.data != nullptr)
    {
        ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        SetWindowIcon(icon);
        UnloadImage(icon);
    }

    TraceLog(LOG_INFO, "[EditorApplication] Application started.");
}

void EditorApplication::OnUpdate(float deltaTime)
{
    if (!m_editor)
        return;

    // Update editor state
    m_editor->Update();

    // Handle editor input
    m_editor->HandleInput();
}

void EditorApplication::OnRender()
{
    if (!m_editor)
        return;

    // Editor rendering
    // BeginFrame() already called in Engine::Render() via RenderManager::BeginFrame()
    // EndFrame() will be called in Engine::Render() via RenderManager::EndFrame()

    // Clear background before 3D scene
    ClearBackground(m_editor->GetClearColor());

    // Render 3D scene for Editor
    auto cameraController = m_editor->GetCameraController();
    if (cameraController)
    {
        BeginMode3D(cameraController->GetCamera());
        cameraController->SetCameraMode(CAMERA_FREE);

        // Render skybox and objects
        m_editor->Render();

        // Draw grid after scene for orientation
        DrawGrid(m_editor->GetGridSize(), 1.0f);

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

void EditorApplication::OnShutdown()
{
    TraceLog(LOG_INFO, "[EditorApplication] Shutting down...");
    // Editor cleans up its own resources in destructor
}
