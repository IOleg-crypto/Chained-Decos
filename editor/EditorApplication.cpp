#include "EditorApplication.h"
#include "core/Engine.h"
#include "core/application/EngineApplication.h"
#include "editor/Editor.h"
#include "editor/mapgui/UIManager.h"
#include "editor/panels/EditorPanelManager.h"
#include "editor/panels/ViewportPanel.h"
#include "scene/camera/core/CameraController.h"

#include "core/ecs/ECSRegistry.h"
#include "project/chaineddecos/GameLayer.h"
#include "scene/resources/model/core/Model.h"

//===============================================
#include "core/events/Event.h"
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

EditorApplication::EditorApplication(int argc, char *argv[])
{
}

EditorApplication::~EditorApplication()
{
    TraceLog(LOG_INFO, "[EditorApplication] Destructor called.");
}

void EditorApplication::OnConfigure(EngineConfig &config)
{
    TraceLog(LOG_INFO, "[EditorApplication] Configuring application...");
    config.windowName = "ChainedEditor";
    config.width = 1600;
    config.height = 900;
}

void EditorApplication::OnRegister()
{
    TraceLog(LOG_INFO, "[EditorApplication] Registering modules and core services...");

    auto &engine = Engine::Instance();
    // Engine handles core services automatically

    TraceLog(LOG_INFO, "[EditorApplication] Editor modules registered.");
}

void EditorApplication::OnStart()
{
    TraceLog(LOG_INFO, "[EditorApplication] Starting application...");

    // Initialize Editor components
    auto camera = std::make_shared<CameraController>();

    // Use engine-provided ModelLoader instead of creating a new one
    auto modelLoader = Engine::Instance().GetModelLoader();

    m_editor = std::make_unique<Editor>(camera, modelLoader);
    camera->SetCameraMode(CAMERA_FREE);

    TraceLog(LOG_INFO, "[EditorApplication] Editor components initialized.");

    // Configure ImGui for Editor (custom settings)
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable docking
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Set up custom font
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/lato/lato-Black.ttf", 16.0f);
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

    static bool wasInPlayMode = false;
    static GameLayer *playLayer = nullptr;
    bool inPlayMode = m_editor->IsInPlayMode();

    if (inPlayMode != wasInPlayMode)
    {
        if (inPlayMode)
        {
            TraceLog(LOG_INFO, "[EditorApplication] Entering Play Mode: Injecting GameLayer");
            if (GetAppRunner())
            {
                playLayer = new GameLayer();
                GetAppRunner()->PushLayer(playLayer);
            }
        }
        else
        {
            TraceLog(LOG_INFO, "[EditorApplication] Exiting Play Mode: Popping GameLayer");
            if (GetAppRunner() && playLayer)
            {
                GetAppRunner()->PopLayer(playLayer);
                playLayer = nullptr;
            }
        }
        wasInPlayMode = inPlayMode;
    }

    // Update editor state
    m_editor->Update();

    // ... rest of OnUpdate

    // Handle editor input
    m_editor->HandleInput();

    // Check if UI requested exit
    if (auto uiManager = dynamic_cast<EditorUIManager *>(m_editor->GetUIManager()))
    {
        if (uiManager->ShouldExit())
        {
            if (auto engine = GetEngine())
            {
                engine->RequestExit();
            }
        }
    }
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

    // Check if we have a viewport panel to render into
    ViewportPanel *viewport = nullptr;
    if (auto panelManager = m_editor->GetPanelManager())
    {
        viewport = panelManager->GetPanel<ViewportPanel>("Viewport");
    }

    if (viewport && viewport->IsVisible())
    {
        viewport->BeginRendering();

        // Render 3D scene for Editor
        if (m_editor->IsInPlayMode())
        {
            // Use Game Camera
            BeginMode3D(RenderManager::Get().GetCamera());
        }
        else
        {
            // Use Editor Camera
            auto &cameraController = m_editor->GetCameraController();
            BeginMode3D(cameraController.GetCamera());
        }

        // Render skybox and objects
        m_editor->Render();

        // Draw grid after scene for orientation
        DrawGrid(m_editor->GetGridSize(), 1.0f);

        EndMode3D();

        viewport->EndRendering();
    }
    else
    {
        // Fallback: rendering directly to backbuffer if no viewport panel
        auto &cameraController = m_editor->GetCameraController();
        BeginMode3D(cameraController.GetCamera());

        m_editor->Render();

        EndMode3D();
    }

    // Begin ImGui frame for Editor UI
    // Note: rlImGuiBegin() must be called here, before rendering ImGui
    // rlImGuiEnd() will be called AFTER EndFrame() but before kernel->Render()
    rlImGuiBegin();

    // Create dockspace over entire viewport
    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpaceOverViewport(dockspaceId, ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);

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

void EditorApplication::OnEvent(ChainedDecos::Event &e)
{
    if (m_editor)
    {
        m_editor->OnEvent(e);
    }
}
