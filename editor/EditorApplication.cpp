#include "EditorApplication.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "core/application/EngineApplication.h"
#include "core/assets/AssetManager.h"
#include "core/input/Input.h"
#include "core/renderer/Renderer.h"
#include "editor/Editor.h"
#include "editor/mapgui/UIManager.h"
#include "editor/panels/EditorPanelManager.h"
#include "editor/panels/ViewportPanel.h"
#include "scene/camera/core/CameraController.h"

#include "project/ChainedDecos/GameLayer.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/systems/UIRenderSystem.h"
#include "scene/resources/font/FontService.h"
#include "scene/resources/model/core/Model.h"

//===============================================
#include "events/Event.h"
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>
#include <rlImGui.h>

// Declare this as the main application entry point
#include "core/application/EntryPoint.h"
DECLARE_APPLICATION(EditorApplication)

// Global/Static Play Mode State
using namespace CHEngine;
static CHD::GameLayer *s_playLayer = nullptr;

EditorApplication::EditorApplication(int argc, char *argv[])
{
}

EditorApplication::~EditorApplication()
{
    CD_INFO("[EditorApplication] Destructor called.");
}

void EditorApplication::OnConfigure(EngineConfig &config)
{
    CD_INFO("[EditorApplication] Configuring application...");
    config.windowName = "ChainedEditor";
    config.width = 1600;
    config.height = 900;
}

void EditorApplication::OnRegister()
{
    CD_INFO("[EditorApplication] Registering modules and core services...");

    auto &engine = Engine::Instance();
    // Engine handles core services automatically

    CD_INFO("[EditorApplication] Editor modules registered.");
}

void EditorApplication::OnStart()
{
    CD_INFO("[EditorApplication] Starting application...");

    // Initialize Editor components
    auto camera = std::make_shared<CameraController>();

    // Use static AssetManager instead of creating a new one

    m_editor = std::make_unique<Editor>(&Engine::Instance());

    // Register editor as a service for GameLayer to access
    std::shared_ptr<IEditor> editorService(m_editor.get(), [](IEditor *) {});
    Engine::Instance().RegisterService<IEditor>(editorService);

    m_editor->GetCameraController().SetCameraMode(CAMERA_FREE);

    CD_INFO("[EditorApplication] Editor components initialized.");

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

        // Load default fonts for UI
        AssetManager::LoadFont("Gantari", PROJECT_ROOT_DIR
                               "/resources/font/gantari/Gantari-VariableFont_wght.ttf");
    }

    // Set window icon
    Image icon = LoadImage(PROJECT_ROOT_DIR "/resources/icons/CHEngineMapEditor.jpg");
    if (icon.data != nullptr)
    {
        ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        SetWindowIcon(icon);
        UnloadImage(icon);
    }

    CD_INFO("[EditorApplication] Application started.");
}

void EditorApplication::OnUpdate(float deltaTime)
{
    if (!m_editor)
        return;

    static bool wasInPlayMode = false;
    bool inPlayMode = m_editor->IsInPlayMode();

    if (inPlayMode != wasInPlayMode)
    {
        if (inPlayMode)
        {
            CD_INFO("[EditorApplication] Entering Play Mode: Injecting GameLayer");
            if (GetAppRunner())
            {
                s_playLayer = new CHD::GameLayer();
                // GetAppRunner()->PushLayer(s_playLayer); // We manually render now
                s_playLayer->OnAttach(); // Manually attach
            }
        }
        else
        {
            CD_INFO("[EditorApplication] Exiting Play Mode: Popping GameLayer");
            if (GetAppRunner() && s_playLayer)
            {
                // GetAppRunner()->PopLayer(s_playLayer);
                s_playLayer->OnDetach();
                delete s_playLayer; // We own it now since we didn't push it
                s_playLayer = nullptr;
            }
        }
        wasInPlayMode = inPlayMode;
    }

    // Update Game Layer if active
    if (s_playLayer && inPlayMode)
    {
        s_playLayer->OnUpdate(deltaTime);
    }

    // Update editor state
    m_editor->Update();

    // ... rest of OnUpdate

    // Handle editor input
    m_editor->HandleInput();

    // Check if UI requested exit
    if (auto uiManager = dynamic_cast<EditorUIManager *>(&m_editor->GetUIManager()))
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

    // Clear background with scene's background color
    Color bgColor = m_editor->GetSceneManager().GetGameScene().GetMapMetaData().backgroundColor;
    ClearBackground(bgColor);

    // Check if we have a viewport panel to render into
    ViewportPanel *viewport = nullptr;
    auto &panelManager = m_editor->GetPanelManager();
    viewport = panelManager.GetPanel<ViewportPanel>("Viewport");

    if (viewport && viewport->IsVisible())
    {
        viewport->BeginRendering();

        // Use Game Camera in Play Mode, Editor Camera in Edit Mode
        Camera3D camera = m_editor->IsInPlayMode() ? Renderer::GetCamera()
                                                   : m_editor->GetCameraController().GetCamera();

        BeginMode3D(camera);
        {
            // This renders the skybox and the map objects
            m_editor->Render();

            if (m_editor->IsInPlayMode())
            {
                // In Play Mode, additionally render active game entities (player)
                if (s_playLayer)
                {
                    s_playLayer->RenderScene();
                }
            }
        }
        EndMode3D();

        // Render UI elements if in UI Design mode
        if (m_editor->GetState().IsUIDesignMode())
        {
            int vpWidth = static_cast<int>(viewport->GetSize().x);
            int vpHeight = static_cast<int>(viewport->GetSize().y);
            CHEngine::UIRenderSystem::Render(vpWidth, vpHeight);
        }
        else if (m_editor->IsInPlayMode() && s_playLayer)
        {
            // Render Game Interface (both HUD and serialized UI)
            int vpWidth = static_cast<int>(viewport->GetSize().x);
            int vpHeight = static_cast<int>(viewport->GetSize().y);
            s_playLayer->RenderUI((float)vpWidth, (float)vpHeight);
        }

        viewport->EndRendering();
    }
    else
    {
        // Fallback: rendering directly to backbuffer if no viewport panel
        auto &cameraController = m_editor->GetCameraController();
        BeginMode3D(cameraController.GetCamera());

        m_editor->Render();

        EndMode3D();

        if (m_editor->IsInPlayMode() && s_playLayer)
        {
            s_playLayer->RenderUI((float)GetScreenWidth(), (float)GetScreenHeight());
        }
    }
}

void EditorApplication::OnImGuiRender()
{
    if (!m_editor)
        return;

    // Create dockspace over entire viewport
    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpaceOverViewport(dockspaceId, ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);

    // Render ImGui interface
    m_editor->RenderImGui();
}

void EditorApplication::OnShutdown()
{
    CD_INFO("[EditorApplication] Shutting down...");
    // Editor cleans up its own resources in destructor
}

void EditorApplication::OnEvent(CHEngine::Event &e)
{
    if (m_editor)
    {
        m_editor->OnEvent(e);

        if (m_editor->IsInPlayMode() && s_playLayer)
        {
            s_playLayer->OnEvent(e);
        }
    }
}
