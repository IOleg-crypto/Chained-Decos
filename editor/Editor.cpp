#include "Editor.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "core/assets/AssetManager.h"
#include "core/input/Input.h"
#include "core/renderer/Renderer.h"
#include "core/scripting/ScriptEngine.h"
#include "editor/mapgui/UIManager.h" // Added for EditorUIManager and UIManagerConfig
#include "editor/panels/AssetBrowserPanel.h"
#include "editor/panels/ConsolePanel.h"
#include "editor/panels/HierarchyPanel.h"
#include "editor/panels/InspectorPanel.h"
#include "editor/panels/ToolbarPanel.h"
#include "editor/panels/UIEditorPanel.h"
#include "editor/panels/ViewportPanel.h"
#include "scene/ecs/ECSRegistry.h"
#include "scene/resources/map/core/SceneLoader.h"


#define PLATFORM_DESKTOP
#include "scene/resources/model/interfaces/IModelLoader.h"

#include <filesystem>
#include <imgui.h>
#include <raymath.h>

using namespace CHEngine;

Editor::Editor(IEngine *engine) : m_engine(engine), m_isInPlayMode(false)
{
    CD_INFO("[Editor] Initializing...");

    // 1. Initialize core services if not already present
    if (!m_engine->GetService<IModelLoader>())
    {
        m_modelLoader = std::make_shared<ModelLoader>();
        m_engine->RegisterService<IModelLoader>(m_modelLoader);
    }
    else
    {
        m_modelLoader = m_engine->GetService<IModelLoader>();
    }

    // 2. Load basic resources
    LoadSpawnTexture();

    // 3. Setup subsystems
    InitializeSubsystems();

    CD_INFO("[Editor] Initialization complete.");
}

Editor::~Editor()
{
}

void Editor::InitializeSubsystems()
{
    // Create concrete managers
    m_projectManager = std::make_unique<ProjectManager>(this);
    m_sceneManager = std::make_unique<::SceneManager>();
    m_selectionManager = std::make_unique<SelectionManager>(); // Fixed: matches default constructor
    m_editorState = std::make_unique<EditorState>();

    // Camera controller
    m_cameraController = CHEngine::Ref<CameraController>(new CameraController());

    // UI and Panels
    m_panelManager = std::make_unique<EditorPanelManager>(this);

    // Register panels
    m_panelManager->AddPanel<ToolbarPanel>(this);
    m_panelManager->AddPanel<ViewportPanel>(this);
    m_panelManager->AddPanel<HierarchyPanel>(this);
    m_panelManager->AddPanel<InspectorPanel>(this);
    m_panelManager->AddPanel<AssetBrowserPanel>(this);
    m_panelManager->AddPanel<ConsolePanel>(this);
    m_panelManager->AddPanel<UIEditorPanel>(this);

    UIManagerConfig config;
    config.editor = this;
    m_uiManager = std::make_unique<EditorUIManager>(config);

    // Link Scripting System with Scene Manager
    ScriptEngine::GetLuaState(); // Ensure initialized if needed, but ScriptEngine should handle it
    Engine::Instance().GetScriptManager().SetSceneManager(m_sceneManager.get());
}

void Editor::Update()
{
    // Update camera controller bypass based on viewport focus/hover
    if (m_cameraController && m_panelManager)
    {
        auto viewport = m_panelManager->GetPanel<ViewportPanel>("Viewport");
        if (viewport)
        {
            bool shouldBypass = viewport->IsFocused() && viewport->IsHovered();
            m_cameraController->SetInputCaptureBypass(shouldBypass);
        }
    }

    // Update camera controller
    if (m_cameraController)
        m_cameraController->Update();
}

void Editor::Render()
{
    // Use the appropriate camera based on current mode
    Camera3D &activeCamera =
        m_isInPlayMode ? Renderer::GetCamera() : m_cameraController->GetCamera();

    bool isUIScene =
        (m_sceneManager->GetGameScene().GetMapMetaData().sceneType == SceneType::UI_MENU);

    if (!isUIScene && m_sceneManager->GetSkybox() && m_sceneManager->GetSkybox()->IsLoaded())
    {
        m_sceneManager->GetSkybox()->UpdateGammaFromConfig();
        m_sceneManager->GetSkybox()->DrawSkybox(activeCamera.position);
    }

    // Render all objects in the scene via SceneManager (only if NOT a UI scene)
    if (!isUIScene)
    {
        const auto &objects = m_sceneManager->GetGameScene().GetMapObjects();
        for (const auto &obj : objects)
        {
            RenderObject(obj);
        }
    }

    // Draw grid only in editor mode and NOT for UI scenes
    if (!m_isInPlayMode && !isUIScene)
    {
        DrawGrid(m_editorState->GetGridSize(), 1.0f);
    }
}

void Editor::RenderObject(const MapObjectData &obj)
{
    // Using simple rendering for now, can be moved to a dedicated EditorRenderer
    bool isSelected = (m_selectionManager->GetSelectedObject() == &obj);

    if (obj.type == MapObjectType::SPAWN_ZONE)
    {
        // Simple spawn zone rendering fallback
        DrawCubeWires(obj.position, obj.scale.x, obj.scale.y, obj.scale.z, RED);
        if (isSelected)
            DrawCubeWires(obj.position, obj.scale.x + 0.1f, obj.scale.y + 0.1f, obj.scale.z + 0.1f,
                          YELLOW);
        return;
    }

    // Simple model/primitive rendering
    if (obj.type == MapObjectType::MODEL && !obj.modelName.empty())
    {
        auto modelOpt = AssetManager::GetModel(obj.modelName);
        if (modelOpt)
        {
            DrawModel(*modelOpt, obj.position, 1.0f, obj.color);
            if (isSelected)
                DrawModelWires(*modelOpt, obj.position, 1.0f, YELLOW);
        }
    }
    else if (obj.type == MapObjectType::CUBE)
    {
        DrawCube(obj.position, obj.scale.x, obj.scale.y, obj.scale.z, obj.color);
        if (isSelected)
            DrawCubeWires(obj.position, obj.scale.x, obj.scale.y, obj.scale.z, YELLOW);
    }
    // ... other primitives
}

void Editor::RenderImGui()
{
    bool welcomeActive = false;
    if (m_uiManager)
    {
        welcomeActive = m_uiManager->IsWelcomeScreenActive();
        m_uiManager->Render();
    }

    if (m_panelManager && !welcomeActive)
    {
        m_panelManager->Render();
    }
}

void Editor::HandleInput()
{
    if (m_uiManager)
    {
        m_uiManager->HandleInput();
    }

    if (m_selectionManager && m_cameraController && !m_isInPlayMode)
    {
        const ImGuiIO &io = ImGui::GetIO();
        bool viewportHovered = false;
        if (m_panelManager)
        {
            auto viewport = m_panelManager->GetPanel<ViewportPanel>("Viewport");
            viewportHovered = (viewport && viewport->IsHovered());
        }

        if (!io.WantCaptureMouse || viewportHovered)
        {
            // Input logic for tools/selection will be handled by sub-managers
        }
    }
}

void Editor::OnEvent(CHEngine::Event &e)
{
    if (m_isInPlayMode)
        return;
    if (m_cameraController)
        m_cameraController->OnEvent(e);
}

void Editor::StartPlayMode()
{
    if (m_isInPlayMode)
        return;
    CD_INFO("[Editor] Starting Play Mode...");
    m_isInPlayMode = true;
}

void Editor::StopPlayMode()
{
    if (!m_isInPlayMode)
        return;
    CD_INFO("[Editor] Stopping Play Mode...");
    m_isInPlayMode = false;
    REGISTRY.clear();
}

bool Editor::IsInPlayMode() const
{
    return m_isInPlayMode;
}
void Editor::BuildGame()
{
    CD_INFO("[Editor] Build initiated...");
}

void Editor::RunGame()
{
    CD_INFO("[Editor] Run initiated...");
}

void Editor::PreloadModelsFromResources()
{
    CD_INFO("[Editor] Preloading models...");
}

void Editor::LoadSpawnTexture()
{
    // Implementation details if any
}

std::string Editor::GetSkyboxAbsolutePath() const
{
    return "";
}

IUIManager &Editor::GetUIManager()
{
    return *m_uiManager;
}

EditorPanelManager &Editor::GetPanelManager()
{
    return *m_panelManager;
}

CHEngine::Ref<IModelLoader> Editor::GetModelLoader()
{
    return m_modelLoader;
}

CameraController &Editor::GetCameraController()
{
    return *m_cameraController;
}

IProjectManager &Editor::GetProjectManager()
{
    return *m_projectManager;
}

ISceneManager &Editor::GetSceneManager()
{
    return *m_sceneManager;
}

ISelectionManager &Editor::GetSelectionManager()
{
    return *m_selectionManager;
}

IEditorState &Editor::GetState()
{
    return *m_editorState;
}
