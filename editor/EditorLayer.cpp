#include "EditorLayer.h"
#include "core/Base.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "core/application/EngineApplication.h"
#include "core/input/Input.h"
#include "editor/utils/EditorStyles.h"
#include <cstdlib>

#include "core/interfaces/ILevelManager.h"
#include "core/interfaces/IPlayer.h"
#include "editor/logic/SceneCloner.h"
#include "imgui_internal.h"
#include "nfd.h"
#include "project/Runtime/RuntimeLayer.h"
#include "raylib.h"
#include "scene/resources/map/SceneSerializer.h"
#include <filesystem>

namespace CHEngine
{
EditorLayer::EditorLayer() : Layer("EditorLayer")
{
}

void EditorLayer::OnAttach()
{
    CD_INFO("EditorLayer attached");

    m_CameraController = std::make_shared<CameraController>();
    m_CameraController->SetCameraMode(CAMERA_FREE); // Better for editor flying
    m_EditorScene = std::make_shared<GameScene>();
    m_ActiveScene = m_EditorScene;

    m_HierarchyPanel = std::make_unique<HierarchyPanel>(m_ActiveScene);
    m_InspectorPanel = std::make_unique<InspectorPanel>();
    m_ViewportPanel = std::make_unique<ViewportPanel>();
    m_AssetBrowserPanel = std::make_unique<AssetBrowserPanel>();
    m_ToolbarPanel = std::make_unique<ToolbarPanel>();
    m_ConsolePanel = std::make_unique<ConsolePanel>();

    m_InspectorPanel->SetSkyboxCallback([this](const std::string &path) { LoadSkybox(path); });

    // Default Scene Content
    m_ActiveScene = m_EditorScene;
    auto &objects = m_ActiveScene->GetMapObjectsMutable();

    // ground
    MapObjectData ground;
    ground.name = "Ground";
    ground.type = MapObjectType::PLANE;
    ground.size = {100, 100};
    ground.color = DARKGRAY;
    objects.push_back(ground);

    // default cube
    MapObjectData cube;
    cube.name = "Default Cube";
    cube.type = MapObjectType::CUBE;
    cube.position = {0, 0.5f, 0};
    cube.color = WHITE;
    objects.push_back(cube);

    m_HierarchyPanel->SetContext(m_ActiveScene);

    // Apply theme
    EditorStyles::ApplyDarkTheme();
}

void EditorLayer::OnDetach()
{
    CD_INFO("EditorLayer detached");
}

void EditorLayer::OnUpdate(float deltaTime)
{
    if (m_SceneState == SceneState::Play)
    {
        // Update Game Logic via Engine modules
        auto levelManager = Engine::Instance().GetService<ILevelManager>();
        if (levelManager)
        {
            levelManager->Update(deltaTime);
        }

        auto player = Engine::Instance().GetService<IPlayer>();
        if (player)
        {
            player->Update(deltaTime);
        }
    }

    // Update logic
    if (m_ViewportPanel)
    {
        bool viewportActive = m_ViewportPanel->IsFocused() || m_ViewportPanel->IsHovered();
        m_CameraController->SetInputCaptureBypass(viewportActive);

        if (viewportActive && m_SceneState == SceneState::Edit)
            m_CameraController->Update();
    }

    // Direct simulation escape check
    if (m_SceneState == SceneState::Play && Input::IsKeyPressed(KEY_BACKSPACE))
    {
        OnSceneStop();
    }
}

void EditorLayer::OnRender()
{
    // This is called by Engine::Render
    // We render the viewport here if needed, or in OnImGuiRender
}

void EditorLayer::OnImGuiRender()
{
    UI_DrawDockspace();
    m_ToolbarPanel->OnImGuiRender(
        m_SceneState, m_RuntimeMode, m_ActiveTool, [this]() { OnScenePlay(); },
        [this]() { OnSceneStop(); }, [this]() { NewScene(); }, [this]() { SaveScene(); },
        [this](Tool t) { SetActiveTool(t); }, [this](RuntimeMode mode) { m_RuntimeMode = mode; });

    // Panels
    if (m_HierarchyPanel && m_HierarchyPanel->IsVisible())
    {
        m_HierarchyPanel->OnImGuiRender(
            m_SelectionType, m_SelectedObjectIndex,
            [this](SelectionType type, int i) { SetSelectedObjectIndex(i, type); },
            [this]() { AddModel(); }, [this](const std::string &type) { AddUIElement(type); });
    }

    if (m_InspectorPanel && m_InspectorPanel->IsVisible())
    {
        if (m_SelectionType == SelectionType::UI_ELEMENT)
            m_InspectorPanel->OnImGuiRender(m_ActiveScene, GetSelectedUIElement());
        else
            m_InspectorPanel->OnImGuiRender(m_ActiveScene, GetSelectedObject());
    }

    if (m_ViewportPanel && m_ViewportPanel->IsVisible())
    {
        std::shared_ptr<CameraController> activeCamera = m_CameraController;
        if (m_SceneState == SceneState::Play)
        {
            auto player = Engine::Instance().GetService<IPlayer>();
            if (player)
                activeCamera = player->GetCameraController();
        }

        m_ViewportPanel->OnImGuiRender(m_ActiveScene, activeCamera, m_SelectedObjectIndex,
                                       m_ActiveTool, [this](int i) { SetSelectedObjectIndex(i); });
    }

    if (m_AssetBrowserPanel && m_AssetBrowserPanel->IsVisible())
        m_AssetBrowserPanel->OnImGuiRender();

    if (m_ConsolePanel && m_ConsolePanel->IsVisible())
        m_ConsolePanel->OnImGuiRender();

    // Editor Modals / Settings
    if (m_ShowProjectSettings)
    {
        ImGui::Begin("Project Settings", &m_ShowProjectSettings);

        if (ImGui::CollapsingHeader("Runtime", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const char *modes[] = {"Embedded", "Standalone"};
            int currentMode = (int)m_RuntimeMode;
            if (ImGui::Combo("Default Runtime Mode", &currentMode, modes, IM_ARRAYSIZE(modes)))
            {
                m_RuntimeMode = (RuntimeMode)currentMode;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(
                    "Embedded: Run inside the editor viewport.\nStandalone: Launch a separate "
                    "process.");
        }

        if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static float gravity[3] = {0.0f, -9.81f, 0.0f};
            ImGui::DragFloat3("Gravity", gravity, 0.1f);
            static bool airResistance = true;
            ImGui::Checkbox("Enable Air Resistance", &airResistance);
        }

        if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static bool vsync = true;
            if (ImGui::Checkbox("VSync", &vsync))
            {
                // Engine::Instance().GetWindow()->SetVSync(vsync);
            }
            static float fov = 60.0f;
            ImGui::SliderFloat("Field of View", &fov, 30.0f, 120.0f);
        }

        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0)))
            m_ShowProjectSettings = false;

        ImGui::End();
    }
}

void EditorLayer::OnEvent(Event &event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<KeyPressedEvent>(CD_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
    dispatcher.Dispatch<MouseButtonPressedEvent>(
        CD_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
}

bool EditorLayer::OnKeyPressed(KeyPressedEvent &e)
{
    // Shortcuts (Hazel style)
    if (e.GetRepeatCount() > 0)
        return false;

    bool control = Input::IsKeyDown(KEY_LEFT_CONTROL) || Input::IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = Input::IsKeyDown(KEY_LEFT_SHIFT) || Input::IsKeyDown(KEY_RIGHT_SHIFT);
    if (control)
    {
        switch (e.GetKeyCode())
        {
        case KEY_N:
            NewScene();
            break;
        case KEY_O:
            OpenScene();
            break;
        case KEY_S:
            if (shift)
                SaveSceneAs();
            else
                SaveScene();
            break;
        }
    }
    else
    {
        switch (e.GetKeyCode())
        {
        case KEY_ESCAPE:
            if (m_SceneState == SceneState::Play)
                OnSceneStop();
            break;
        case KEY_Q:
            SetActiveTool(Tool::SELECT);
            break;
        case KEY_W:
            SetActiveTool(Tool::MOVE);
            break;
        case KEY_E:
            SetActiveTool(Tool::ROTATE);
            break;
        case KEY_R:
            SetActiveTool(Tool::SCALE);
            break;
        case KEY_DELETE:
            if (m_SelectedObjectIndex >= 0 && m_ActiveScene)
            {
                auto &objects = m_ActiveScene->GetMapObjectsMutable();
                if (m_SelectedObjectIndex < (int)objects.size())
                {
                    objects.erase(objects.begin() + m_SelectedObjectIndex);
                    m_SelectedObjectIndex = -1;
                }
            }
            break;
        }
    }

    return false;
}

bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent &e)
{
    // Object picking is now handled by ViewportPanel::OnImGuiRender
    // to ensure correct coordinate mapping within the render texture's 3D context.
    return false;
}

void EditorLayer::UI_DrawDockspace()
{
    static bool opt_fullscreen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + 32.0f));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y - 32.0f));
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");

        // Initial layout setup
        static bool first_time = true;
        if (first_time)
        {
            first_time = false;
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

            ImGuiID dock_main_id = dockspace_id;
            ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f,
                                                                nullptr, &dock_main_id);
            ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f,
                                                                 nullptr, &dock_main_id);
            ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f,
                                                               nullptr, &dock_main_id);
            ImGuiID dock_id_left_bottom = ImGui::DockBuilderSplitNode(
                dock_id_left, ImGuiDir_Down, 0.50f, nullptr, &dock_id_left);

            ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
            ImGui::DockBuilderDockWindow("Scene hierarchy", dock_id_left);
            ImGui::DockBuilderDockWindow("File manager", dock_id_left_bottom);
            ImGui::DockBuilderDockWindow("Properties", dock_id_right);
            ImGui::DockBuilderDockWindow("Console", dock_id_bottom);
            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    UI_DrawMenuBar();

    ImGui::End();
}

void EditorLayer::UI_DrawMenuBar()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N"))
                NewScene();
            if (ImGui::MenuItem("Open...", "Ctrl+O"))
                OpenScene();
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S"))
                SaveScene();
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
                SaveSceneAs();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
                Engine::Instance().RequestExit();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false))
            {
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false))
            {
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            if (m_HierarchyPanel &&
                ImGui::MenuItem("Hierarchy", nullptr, m_HierarchyPanel->IsVisible()))
                m_HierarchyPanel->SetVisible(!m_HierarchyPanel->IsVisible());
            if (m_InspectorPanel &&
                ImGui::MenuItem("Inspector", nullptr, m_InspectorPanel->IsVisible()))
                m_InspectorPanel->SetVisible(!m_InspectorPanel->IsVisible());
            if (m_ViewportPanel &&
                ImGui::MenuItem("Viewport", nullptr, m_ViewportPanel->IsVisible()))
                m_ViewportPanel->SetVisible(!m_ViewportPanel->IsVisible());
            if (m_AssetBrowserPanel &&
                ImGui::MenuItem("Asset Browser", nullptr, m_AssetBrowserPanel->IsVisible()))
                m_AssetBrowserPanel->SetVisible(!m_AssetBrowserPanel->IsVisible());
            if (m_ConsolePanel && ImGui::MenuItem("Console", nullptr, m_ConsolePanel->IsVisible()))
                m_ConsolePanel->SetVisible(!m_ConsolePanel->IsVisible());
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Project"))
        {
            if (ImGui::MenuItem("Project Settings"))
                m_ShowProjectSettings = true;
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About"))
            {
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void EditorLayer::OnScenePlay()
{
    m_SceneState = SceneState::Play;
    CD_INFO("Scene Play started (Mode: %s)",
            m_RuntimeMode == RuntimeMode::Standalone ? "Standalone" : "Embedded");

    // 1. Find Spawn Point
    Vector3 spawnPos = {0, 5, 0}; // Default
    for (const auto &obj : m_ActiveScene->GetMapObjects())
    {
        if (obj.type == MapObjectType::SPAWN_ZONE)
        {
            spawnPos = obj.position;
            CD_INFO("Found Spawn Zone at (%.2f, %.2f, %.2f)", spawnPos.x, spawnPos.y, spawnPos.z);
            break;
        }
    }

    // 2. Save current state to temp (.chscene)
    if (m_ActiveScene)
    {
        std::string tempPath = SceneCloner::GetTempPath();
        if (tempPath.find(".json") != std::string::npos)
            tempPath.replace(tempPath.find(".json"), 5, ".chscene");

        SceneSerializer serializer(m_ActiveScene);
        if (serializer.SerializeBinary(tempPath))
        {
            if (m_RuntimeMode == RuntimeMode::Standalone)
            {
                // Standalone Runtime process
                std::string cmd = "start bin/Runtime.exe --map \"" + tempPath + "\" --skip-menu";
                CD_INFO("Launching standalone runtime: %s", cmd.c_str());
                std::system(cmd.c_str());
                CD_INFO("Standalone runtime process started");
            }
            else
            {
                // Embedded Runtime
                CD_INFO("Launching embedded runtime...");
                if (GetAppRunner())
                {
                    m_RuntimeLayer = new CHD::RuntimeLayer();
                    GetAppRunner()->PushLayer(m_RuntimeLayer);
                }
                else
                {
                    CD_ERROR("Cannot launch embedded runtime: AppRunner is null!");
                }
            }
        }
    }
}

void EditorLayer::OnSceneStop()
{
    m_SceneState = SceneState::Edit;
    CD_INFO("Scene Play stopped");

    // 0. Pop RuntimeLayer if embedded
    if (m_RuntimeLayer != nullptr && GetAppRunner() != nullptr)
    {
        GetAppRunner()->PopLayer(m_RuntimeLayer);
        // LayerStack::PopLayer does NOT delete the layer, we must do it if we created it with new
        delete m_RuntimeLayer;
        m_RuntimeLayer = nullptr;
    }

    // 1. Restore Active Scene to Editor
    m_ActiveScene = m_EditorScene;
    m_HierarchyPanel->SetContext(m_ActiveScene);

    // 2. Clear editor scene before restoration to prevent duplicates
    m_EditorScene->Cleanup();

    // 3. Restore original scene state from temp backup
    std::string tempPath = SceneCloner::GetTempPath();
    if (tempPath.find(".json") != std::string::npos)
        tempPath.replace(tempPath.find(".json"), 5, ".chscene");

    SceneSerializer serializer(m_EditorScene);
    if (serializer.DeserializeBinary(tempPath))
    {
        CD_INFO("Editor scene restored from backup binary");
        SceneLoader().LoadSkyboxForScene(*m_EditorScene);
    }

    EnableCursor();
}

MapObjectData *EditorLayer::GetSelectedObject()
{
    if (m_SelectionType != SelectionType::WORLD_OBJECT || m_SelectedObjectIndex < 0 ||
        !m_ActiveScene)
        return nullptr;

    auto &objects = m_ActiveScene->GetMapObjectsMutable();
    if (m_SelectedObjectIndex >= (int)objects.size())
        return nullptr;

    return &objects[m_SelectedObjectIndex];
}

UIElementData *EditorLayer::GetSelectedUIElement()
{
    if (m_SelectionType != SelectionType::UI_ELEMENT || m_SelectedObjectIndex < 0 || !m_ActiveScene)
        return nullptr;

    auto &elements = m_ActiveScene->GetUIElementsMutable();
    if (m_SelectedObjectIndex >= (int)elements.size())
        return nullptr;

    return &elements[m_SelectedObjectIndex];
}

void EditorLayer::UI_DrawToolbar()
{
    // MOVED TO ToolbarPanel
}

void EditorLayer::NewScene()
{
    m_EditorScene = std::make_shared<GameScene>();
    m_ActiveScene = m_EditorScene;
    m_ScenePath = "";
    m_HierarchyPanel->SetContext(m_ActiveScene);
}

void EditorLayer::OpenScene()
{
    nfdchar_t *outPath = nullptr;
    nfdfilteritem_t filterItem[2] = {{"Chained Decos Scene", "chscene"},
                                     {"Legacy JSON Scene", "json"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 2, nullptr);

    if (result == NFD_OKAY)
    {
        m_EditorScene = std::make_shared<GameScene>();
        SceneSerializer serializer(m_EditorScene);

        std::string path(outPath);
        if (path.find(".chscene") != std::string::npos)
            serializer.DeserializeBinary(path);
        else
            serializer.DeserializeJson(path);

        m_ActiveScene = m_EditorScene;
        m_ScenePath = outPath;
        m_HierarchyPanel->SetContext(m_ActiveScene);
        SceneLoader().LoadSkyboxForScene(*m_EditorScene);
        NFD_FreePath(outPath);
    }
}

void EditorLayer::SaveScene()
{
    if (m_ScenePath.empty())
    {
        SaveSceneAs();
    }
    else
    {
        SceneSerializer serializer(m_EditorScene);
        serializer.SerializeBinary(m_ScenePath);
    }
}

void EditorLayer::SaveSceneAs()
{
    nfdchar_t *outPath = nullptr;
    nfdfilteritem_t filterItem[1] = {{"Chained Decos Scene", "json"}};
    nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, nullptr, "scene.json");

    if (result == NFD_OKAY)
    {
        m_ScenePath = outPath;
        if (m_ScenePath.find(".chscene") == std::string::npos)
            m_ScenePath += ".chscene";

        SceneSerializer serializer(m_EditorScene);
        serializer.SerializeBinary(m_ScenePath);
        NFD_FreePath(outPath);
    }
}
void EditorLayer::AddModel()
{
    nfdchar_t *outPath = nullptr;
    nfdfilteritem_t filterItem[2] = {{"3D Models", "obj,glb,gltf"}, {"All Files", "*"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 2, nullptr);

    if (result == NFD_OKAY)
    {
        std::filesystem::path fullPath(outPath);
        std::string filename = fullPath.filename().string();

        // 1. Ensure model is in scene's cache
        auto &models = m_ActiveScene->GetMapModelsMutable();
        if (models.find(filename) == models.end())
        {
            Model model = LoadModel(outPath);
            if (model.meshCount > 0)
            {
                models[filename] = model;
                CD_INFO("Loaded model: %s", filename.c_str());
            }
            else
            {
                CD_ERROR("Failed to load model: %s", outPath);
            }
        }

        // 2. Create MapObjectData
        MapObjectData obj;
        obj.name = filename;
        obj.type = MapObjectType::MODEL;
        obj.modelName = filename;
        obj.position = {0, 0, 0};
        obj.scale = {1, 1, 1};
        obj.color = WHITE;

        auto &objects = m_ActiveScene->GetMapObjectsMutable();
        objects.push_back(obj);
        m_SelectionType = SelectionType::WORLD_OBJECT;
        m_SelectedObjectIndex = (int)objects.size() - 1;

        NFD_FreePath(outPath);
    }
}

void EditorLayer::AddUIElement(const std::string &type)
{
    if (!m_ActiveScene)
        return;

    UIElementData el;
    el.name = "New " + type;
    el.type = type;
    el.position = {100, 100};
    el.size = {120, 50};

    if (type == "text")
    {
        el.text = "New Text";
        el.fontSize = 20;
    }
    else if (type == "button")
    {
        el.text = "Button";
    }

    auto &elements = m_ActiveScene->GetUIElementsMutable();
    elements.push_back(el);
    m_SelectionType = SelectionType::UI_ELEMENT;
    m_SelectedObjectIndex = (int)elements.size() - 1;
}

void EditorLayer::LoadSkybox(const std::string &path)
{
    if (path.empty())
    {
        nfdfilteritem_t filterItem[1] = {{"Skybox Texture (HDR, PNG, JPG)", "hdr,png,jpg,jpeg"}};
        nfdchar_t *outPath = nullptr;
        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

        if (result == NFD_OKAY)
        {
            std::string selectedPath = outPath;
            NFD_FreePath(outPath);
            ApplySkybox(selectedPath);
        }
    }
    else
    {
        ApplySkybox(path);
    }
}

void EditorLayer::ApplySkybox(const std::string &path)
{
    auto skybox = std::make_shared<Skybox>();
    skybox->Init();
    skybox->LoadMaterialTexture(path);
    m_ActiveScene->SetSkyBox(skybox);
    m_ActiveScene->GetMapMetaDataMutable().skyboxTexture = path;
    CD_INFO("Skybox applied: %s", path.c_str());
}
} // namespace CHEngine
