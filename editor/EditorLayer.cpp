#include "EditorLayer.h"
#include "components/physics/collision/core/CollisionManager.h"
#include "core/Base.h"
#include "core/Log.h"
#include "core/application/Application.h"
#include "core/input/Input.h"
#include "core/renderer/Renderer.h"
#include "editor/panels/ConsolePanel.h"
#include "editor/utils/EditorStyles.h"
#include "editor/utils/ProcessUtils.h"
#include "events/MouseEvent.h"
#include "scene/MapManager.h"
#include "scene/ecs/components/PhysicsData.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/core/IDComponent.h"
#include "scene/ecs/components/core/TagComponent.h"
#include <cstdlib>

#include <fstream>

#include "core/interfaces/IPlayer.h"
#include "core/physics/Physics.h"
#include "editor/logic/SceneCloner.h"
#include "editor/logic/undo/AddObjectCommand.h"
#include "editor/logic/undo/DeleteObjectCommand.h"
#include "editor/logic/undo/ModifyObjectCommand.h"
#include "editor/logic/undo/TransformCommand.h"
#include "imgui_internal.h"
#include "nfd.h"
#include "raylib.h"
#include "runtime/RuntimeLayer.h"
#include "scene/core/SceneSerializer.h"
#include "scene/main/LevelManager.h"
#include "scene/resources/map/SceneSerializer.h"
#include "scene/resources/model/Model.h"
#include <filesystem>

namespace CHEngine
{
EditorLayer::EditorLayer() : Layer("EditorLayer")
{
}

// =========================================================================
// Layer Lifecycle
// =========================================================================

void EditorLayer::OnAttach()
{
    CD_INFO("EditorLayer attached");

    // Register Raylib log callback to redirect logs to ConsolePanel
    SetTraceLogCallback(
        [](int logLevel, const char *text, va_list args)
        {
            char buffer[4096];
            vsnprintf(buffer, sizeof(buffer), text, args);

            LogMessage::Level level = LogMessage::Level::Info;
            switch (logLevel)
            {
            case LOG_WARNING:
                level = LogMessage::Level::Warn;
                break;
            case LOG_ERROR:
            case LOG_FATAL:
                level = LogMessage::Level::Error;
                break;
            default:
                level = LogMessage::Level::Info;
                break;
            }

            ConsolePanel::AddLog(buffer, level);
        });

    // Initialize new Scene system
    m_Scene = std::make_shared<Scene>("EditorScene");
    CD_INFO("[EditorLayer] Created editor scene: %s", m_Scene->GetName().c_str());

    if (LevelManager::IsInitialized())
        LevelManager::SetActiveScene(m_Scene);

    // Create default entities using raw entt::entity (temporary workaround for circular dependency)
    auto &registry = m_Scene->GetRegistry();

    // Legacy GameScene initialization
    m_ProjectManager.SetSceneChangedCallback(
        [this](const std::shared_ptr<CHEngine::GameScene> &scene)
        {
            if (MapManager::IsInitialized())
            {
                MapManager::SetCurrentScene(scene);
            }
            if (m_HierarchyPanel)
                m_HierarchyPanel->SetContext(scene);
        });

    m_HierarchyPanel = std::make_unique<HierarchyPanel>(MapManager::GetCurrentScene());
    m_HierarchyPanel->SetSceneContext(m_Scene); // Connect to new Scene system
    m_InspectorPanel = std::make_unique<InspectorPanel>();
    m_InspectorPanel->SetPropertyChangeCallback(
        [this](int index, const MapObjectData &old, const MapObjectData &now)
        { CD_INFO("Object '%s' properties modified", now.name.c_str()); });
    m_ViewportPanel = std::make_unique<ViewportPanel>();
    m_AssetBrowserPanel = std::make_unique<AssetBrowserPanel>();
    m_ToolbarPanel = std::make_unique<ToolbarPanel>();
    m_MenuBarPanel = std::make_unique<MenuBarPanel>();
    m_ConsolePanel = std::make_unique<ConsolePanel>();

    m_InspectorPanel->SetSkyboxCallback([this](const std::string &path) { LoadSkybox(path); });

    // Default Scene Content (legacy)
    auto activeScene = MapManager::GetCurrentScene();
    auto &objects = activeScene->GetMapObjectsMutable();

    // Project Browser Initialization
    m_ProjectBrowserPanel = std::make_unique<ProjectBrowserPanel>();
    m_ProjectBrowserPanel->SetOnCreateProject(
        [this](const std::string &name, const std::string &location)
        { NewProject(name, location); });
    m_ProjectBrowserPanel->SetOnOpenProject([this](const std::string &path) { OpenProject(path); });

    // Apply theme
    EditorStyles::ApplyDarkTheme();
}

void EditorLayer::OnDetach()
{
    CD_INFO("EditorLayer detached");
}

void EditorLayer::OnUpdate(float deltaTime)
{
    if (m_SimulationManager.GetSceneState() == SceneState::Play)
    {
        // Update Game Logic via Engine modules
        if (LevelManager::IsInitialized())
        {
            LevelManager::Update(deltaTime);
        }

        // Note: Player update is now handled by ECS systems in RuntimeLayer
    }

    // Update logic
    if (m_ViewportPanel)
    {
        bool viewportActive = m_ViewportPanel->IsFocused() || m_ViewportPanel->IsHovered();
        m_EditorCamera.SetViewportSize(m_ViewportPanel->GetSize().x, m_ViewportPanel->GetSize().y);

        if (viewportActive && m_SimulationManager.GetSceneState() == SceneState::Edit)
            m_EditorCamera.OnUpdate(deltaTime);
    }

    // Simulation input handling
    if (m_SimulationManager.GetSceneState() == SceneState::Play)
    {
        // Toggle cursor with ESCAPE
        if (Input::IsKeyPressed(KEY_ESCAPE))
        {
            m_CursorLocked = !m_CursorLocked;
            if (m_CursorLocked)
                DisableCursor();
            else
                EnableCursor();
        }

        // Emergency stop with BACKSPACE
        if (Input::IsKeyPressed(KEY_BACKSPACE))
        {
            OnSceneStop();
            return;
        }
    }

    // Update New Scene Architecture
    if (m_Scene)
    {
        m_Scene->OnUpdateEditor(deltaTime);

        // Sync Scene Entities to Physics System
        if (CollisionManager::IsInitialized())
        {
            auto &registry = m_Scene->GetRegistry();
            auto view = registry.view<TransformComponent, CollisionComponent>();

            view.each(
                [&](auto entity, auto &transform, auto &collision)
                {
                    if (collision.collider)
                    {
                        // Update collider position from transform
                        collision.collider->Update(transform.position,
                                                   collision.collider->GetSize());
                    }
                });
        }
    }
}

// =========================================================================
// Rendering & UI
// =========================================================================

void EditorLayer::OnRender()
{
}

void EditorLayer::OnImGuiRender()
{
    UI_DrawDockspace();
    m_ToolbarPanel->OnImGuiRender(
        m_SimulationManager.GetSceneState(), m_SimulationManager.GetRuntimeMode(), m_ActiveTool,
        [this]() { OnScenePlay(); }, [this]() { OnSceneStop(); }, [this]() { NewScene(); },
        [this]() { SaveScene(); }, [this](Tool t) { SetActiveTool(t); },
        [this](RuntimeMode mode) { m_SimulationManager.SetRuntimeMode(mode); });

    // Project Browser Logic
    if (m_ShowProjectBrowser)
    {
        if (m_ProjectBrowserPanel)
        {
            m_ProjectBrowserPanel->OnImGuiRender();
        }
        return; // Don't show other panels if no project is active
    }

    // Panels
    if (m_HierarchyPanel && m_HierarchyPanel->IsVisible())
    {
        m_HierarchyPanel->OnImGuiRender(
            m_SelectionManager.GetSelectionType(), m_SelectionManager.GetSelectedIndex(),
            [this](SelectionType type, int i) { m_SelectionManager.SetSelection(i, type); },
            [this]() { AddModel(); }, [this](const std::string &type) { AddUIElement(type); },
            [this](int i) { DeleteObject(i); }, m_SelectionManager.GetSelectedEntity(),
            [this](entt::entity e) { m_SelectionManager.SetEntitySelection(e); },
            [this]() { CreateEntity(); }, [this](entt::entity e) { DeleteEntity(e); });

        m_InspectorPanel->SetPropertyChangeCallback(
            [this](int index, const MapObjectData &oldData, const MapObjectData &newData)
            {
                // If index is -1, use currently selected object index
                int actualIndex =
                    (index == -1) ? m_SelectionManager.GetSelectedObjectIndex() : index;

                if (actualIndex != -1)
                {
                    auto activeScene = MapManager::GetCurrentScene();
                    // Ensure texture is loaded if it changed
                    if (newData.texturePath != oldData.texturePath && !newData.texturePath.empty())
                    {
                        auto &textures = activeScene->GetMapTexturesMutable();
                        if (textures.find(newData.texturePath) == textures.end())
                        {
                            Texture2D tex = LoadTexture(newData.texturePath.c_str());
                            if (tex.id != 0)
                            {
                                textures[newData.texturePath] = tex;
                            }
                        }
                    }

                    m_CommandHistory.PushCommand(std::make_unique<ModifyObjectCommand>(
                        activeScene, actualIndex, oldData, newData));
                }
            });
    }

    if (m_InspectorPanel && m_InspectorPanel->IsVisible() &&
        m_SimulationManager.GetSceneState() == SceneState::Edit)
    {
        SelectionType selectionType = m_SelectionManager.GetSelectionType();
        if (selectionType == SelectionType::ENTITY)
            m_InspectorPanel->OnImGuiRender(m_Scene, m_SelectionManager.GetSelectedEntity());
        else if (selectionType == SelectionType::UI_ELEMENT)
            m_InspectorPanel->OnImGuiRender(MapManager::GetCurrentScene(), GetSelectedUIElement());
        else
            m_InspectorPanel->OnImGuiRender(MapManager::GetCurrentScene(),
                                            m_SelectionManager.GetSelectedObjectIndex(),
                                            GetSelectedObject());
    }

    // Active Camera for Rendering
    Camera3D activeCamera;
    if (m_SimulationManager.GetSceneState() == SceneState::Edit)
        activeCamera = m_EditorCamera.GetCamera();
    else                                                // Play mode
        activeCamera = CHEngine::Renderer::GetCamera(); // Use actual runtime camera

    if (m_ViewportPanel && m_ViewportPanel->IsVisible())
    {
        ImVec2 viewportSize = m_ViewportPanel->GetSize();
        m_EditorCamera.SetViewportSize(viewportSize.x, viewportSize.y);

        m_ViewportPanel->OnImGuiRender(
            m_SimulationManager.GetSceneState(), m_SelectionManager.GetSelectionType(),
            MapManager::GetCurrentScene(), m_Scene, activeCamera,
            m_SelectionManager.GetSelectedIndex(), m_ActiveTool, [this](int i)
            { m_SelectionManager.SetSelection(i, SelectionType::WORLD_OBJECT); }, &m_CommandHistory,
            [this](const std::string &path, const Vector3 &pos) { OnAssetDropped(path, pos); });
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
            int currentMode = (int)m_SimulationManager.GetRuntimeMode();
            if (ImGui::Combo("Default Runtime Mode", &currentMode, modes, IM_ARRAYSIZE(modes)))
            {
                m_SimulationManager.SetRuntimeMode((RuntimeMode)currentMode);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip(
                    "Embedded: Run inside the editor viewport.\nStandalone: Launch a separate "
                    "process.");
        }

        // if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
        // {
        //     static float gravity[3] = {0.0f, -9.81f, 0.0f};
        //     ImGui::DragFloat3("Gravity", gravity, 0.1f);
        //     static bool airResistance = true;
        //     ImGui::Checkbox("Enable Air Resistance", &airResistance);
        // }

        if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
        {
            static bool vsync = true;
            if (ImGui::Checkbox("VSync", &vsync))
            {
                Application::Get().GetWindow().SetVSync(vsync);
            }
            static float fov = 60.0f;
            ImGui::SliderFloat("Field of View", &fov, 30.0f, 120.0f);
            MouseScrolledEvent e(0.0f, fov);
            m_EditorCamera.OnEvent(e);
        }

        ImGui::Separator();
        if (ImGui::Button("Close", ImVec2(120, 0)))
            m_ShowProjectSettings = false;

        ImGui::End();
    }
}

// =========================================================================
// Events & Input
// =========================================================================

void EditorLayer::OnEvent(Event &event)
{
    if (m_SimulationManager.GetSceneState() == SceneState::Edit)
        m_EditorCamera.OnEvent(event);

    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<KeyPressedEvent>(CD_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
    dispatcher.Dispatch<MouseButtonPressedEvent>(
        CD_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
}

bool EditorLayer::OnKeyPressed(KeyPressedEvent &e)
{
    // Shortcuts
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
        case KEY_Z:
            m_CommandHistory.Undo();
            break;
        case KEY_Y:
            m_CommandHistory.Redo();
            break;
        }
    }
    else
    {
        switch (e.GetKeyCode())
        {
        case KEY_ESCAPE:
            if (m_SimulationManager.GetSceneState() == SceneState::Play)
                OnSceneStop();
            break;
        case KEY_DELETE:
        {
            SelectionType type = m_SelectionManager.GetSelectionType();
            if (type == SelectionType::ENTITY)
            {
                DeleteEntity(m_SelectionManager.GetSelectedEntity());
            }
            else if (type == SelectionType::WORLD_OBJECT)
            {
                DeleteObject(m_SelectionManager.GetSelectedIndex());
            }
            else if (type == SelectionType::UI_ELEMENT)
            {
                auto activeScene = MapManager::GetCurrentScene();
                if (m_SelectionManager.GetSelectedIndex() >= 0 && activeScene)
                {
                    auto &elements = activeScene->GetUIElementsMutable();
                    if (m_SelectionManager.GetSelectedIndex() < (int)elements.size())
                    {
                        elements.erase(elements.begin() + m_SelectionManager.GetSelectedIndex());
                        m_SelectionManager.ClearSelection();
                    }
                }
            }
            break;
        }
        case KEY_Q:
            if (m_SimulationManager.GetSceneState() == SceneState::Edit)
                SetActiveTool(Tool::SELECT);
            break;
        case KEY_W:
            if (m_SimulationManager.GetSceneState() == SceneState::Edit)
                SetActiveTool(Tool::MOVE);
            break;
        case KEY_E:
            if (m_SimulationManager.GetSceneState() == SceneState::Edit)
                SetActiveTool(Tool::ROTATE);
            break;
        case KEY_R:
            if (m_SimulationManager.GetSceneState() == SceneState::Edit)
                SetActiveTool(Tool::SCALE);
            break;
        }
    }

    return false;
}

bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent &e)
{
    return false;
}

void EditorLayer::UI_DrawDockspace()
{
    static bool opt_fullscreen = true;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

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

    // Menu Bar
    PanelVisibility visibility;
    visibility.Hierarchy = m_HierarchyPanel && m_HierarchyPanel->IsVisible();
    visibility.Inspector = m_InspectorPanel && m_InspectorPanel->IsVisible();
    visibility.Viewport = m_ViewportPanel && m_ViewportPanel->IsVisible();
    visibility.AssetBrowser = m_AssetBrowserPanel && m_AssetBrowserPanel->IsVisible();
    visibility.Console = m_ConsolePanel && m_ConsolePanel->IsVisible();

    MenuBarCallbacks callbacks;
    // --- Project Actions ---
    callbacks.OnNewProject = [this]()
    {
        m_ShowProjectBrowser = true;
        if (m_ProjectBrowserPanel)
            m_ProjectBrowserPanel->OpenCreateDialog();
    };
    callbacks.OnOpenProject = [this]()
    {
        nfdfilteritem_t filterItem[1] = {{"CHEngine Project", "chproject"}};
        nfdchar_t *outPath = nullptr;
        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);
        if (result == NFD_OKAY)
        {
            OpenProject(outPath);
            NFD_FreePath(outPath);
        }
    };
    callbacks.OnCloseProject = [this]() { CloseProject(); };

    // --- Scene Actions ---
    callbacks.OnNew = [this]() { NewScene(); };
    callbacks.OnOpen = [this]() { OpenScene(); };
    callbacks.OnSave = [this]() { SaveScene(); };
    callbacks.OnSaveAs = [this]() { SaveSceneAs(); };
    callbacks.OnPlayInRuntime = [this]() { PlayInRuntime(); };
    callbacks.OnExit = []() { Application::Get().Close(); };

    callbacks.OnUndo = [this]() { m_CommandHistory.Undo(); };
    callbacks.OnRedo = [this]() { m_CommandHistory.Redo(); };
    callbacks.CanUndo = [this]() { return m_CommandHistory.CanUndo(); };
    callbacks.CanRedo = [this]() { return m_CommandHistory.CanRedo(); };
    callbacks.TogglePanel = [this](const char *name)
    {
        std::string panelName = name;
        if (panelName == "Hierarchy")
            m_HierarchyPanel->SetVisible(!m_HierarchyPanel->IsVisible());
        else if (panelName == "Inspector")
            m_InspectorPanel->SetVisible(!m_InspectorPanel->IsVisible());
        else if (panelName == "Viewport")
            m_ViewportPanel->SetVisible(!m_ViewportPanel->IsVisible());
        else if (panelName == "Asset Browser")
            m_AssetBrowserPanel->SetVisible(!m_AssetBrowserPanel->IsVisible());
        else if (panelName == "Console")
            m_ConsolePanel->SetVisible(!m_ConsolePanel->IsVisible());
    };
    callbacks.OnShowProjectSettings = [this]() { m_ShowProjectSettings = true; };

    m_MenuBarPanel->OnImGuiRender(visibility, callbacks);

    ImGui::End();
}

// =========================================================================
// Scene Commands
// =========================================================================

void EditorLayer::OnScenePlay()
{
    auto activeScene = MapManager::GetCurrentScene();
    m_SimulationManager.OnScenePlay(activeScene, activeScene, m_Scene, &m_RuntimeLayer,
                                    &Application::Get());
}

void EditorLayer::OnSceneStop()
{
    auto activeScene = MapManager::GetCurrentScene();
    m_SimulationManager.OnSceneStop(activeScene, activeScene, m_Scene, &m_RuntimeLayer,
                                    &Application::Get());
}

MapObjectData *EditorLayer::GetSelectedObject()
{
    return m_SelectionManager.GetSelectedObject(MapManager::GetCurrentScene());
}

UIElementData *EditorLayer::GetSelectedUIElement()
{
    return m_SelectionManager.GetSelectedUIElement(MapManager::GetCurrentScene());
}

void EditorLayer::NewScene()
{
    m_ProjectManager.NewScene();
}

void EditorLayer::OpenScene()
{
    if (m_ProjectManager.OpenScene())
    {
        std::string path = m_ProjectManager.GetScenePath();
        if (path.find(".chscene") != std::string::npos)
        {
            // Check if file is binary (starts with CHSC)
            // If so, ProjectManager has already loaded it via SceneSerializer (binary)
            // We only need to use ECSSceneSerializer for YAML files (new format)

            std::ifstream file(path, std::ios::binary);
            char magic[4];
            if (file.is_open() && file.read(magic, 4))
            {
                if (magic[0] == 'C' && magic[1] == 'H' && magic[2] == 'S' && magic[3] == 'C')
                {
                    CD_CORE_INFO(
                        "EditorLayer detecting binary scene '%s', skipping YAML deserialization.",
                        path.c_str());
                    // Binary file, do NOT try to parse as YAML
                    return;
                }
            }

            // If not binary, try YAML deserialization (migration path)
            ECSSceneSerializer ecsSerializer(m_Scene);
            ecsSerializer.Deserialize(path);
        }
    }
}

void EditorLayer::SaveScene()
{
    m_ProjectManager.SaveScene();
    std::string path = m_ProjectManager.GetScenePath();
    if (!path.empty())
    {
        // For now, don't overwrite binary scenes with YAML unless explicitly requested
        // or ensure we really want to switch formats.
        // ECSSceneSerializer ecsSerializer(m_Scene);
        // ecsSerializer.Serialize(path);
    }
}

void EditorLayer::SaveSceneAs()
{
    m_ProjectManager.SaveSceneAs();
    std::string path = m_ProjectManager.GetScenePath();
    if (!path.empty())
    {
        ECSSceneSerializer ecsSerializer(m_Scene);
        ecsSerializer.Serialize(path);
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

        auto activeScene = MapManager::GetCurrentScene();
        auto &models = activeScene->GetMapModelsMutable();
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

        MapObjectData obj;
        obj.name = filename;
        obj.type = MapObjectType::MODEL;
        obj.modelName = filename;
        obj.position = {0, 0, 0};
        obj.scale = {1, 1, 1};
        obj.color = WHITE;

        auto &objects = activeScene->GetMapObjectsMutable();
        objects.push_back(obj);
        m_SelectionManager.SetSelection((int)objects.size() - 1, SelectionType::WORLD_OBJECT);

        NFD_FreePath(outPath);
    }
}

// =========================================================================
// Entity/Object Management
// =========================================================================

void EditorLayer::AddUIElement(const std::string &type)
{
    auto activeScene = MapManager::GetCurrentScene();
    if (!activeScene)
        return;

    UIElementData el;
    el.name = "element_" + std::to_string(rand() % 1000);
    el.type = type;
    el.position = {m_ViewportSize.x * 0.5f, m_ViewportSize.y * 0.5f};
    el.size = {100, 40};
    el.anchor = 0;
    el.pivot = {0.5f, 0.5f};

    if (type == "Button")
    {
        el.text = "Button";
    }

    auto &elements = activeScene->GetUIElementsMutable();
    elements.push_back(el);
    m_SelectionManager.SetSelection((int)elements.size() - 1, SelectionType::UI_ELEMENT);
}

// =========================================================================
// Environment & Skybox
// =========================================================================

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
    auto activeScene = MapManager::GetCurrentScene();
    auto skybox = std::make_shared<Skybox>();
    skybox->Init();
    skybox->LoadMaterialTexture(path);
    activeScene->SetSkyBox(skybox);
    activeScene->GetMapMetaDataMutable().skyboxTexture = path;
    CD_INFO("Skybox applied: %s", path.c_str());
}

void EditorLayer::AddObject(const MapObjectData &data)
{
    auto activeScene = MapManager::GetCurrentScene();
    if (!activeScene)
        return;

    auto cmd = std::make_unique<AddObjectCommand>(activeScene, data);
    m_CommandHistory.PushCommand(std::move(cmd));
    CD_INFO("Added legacy object: %s", data.name.c_str());
}

void EditorLayer::DeleteObject(int index)
{
    auto activeScene = MapManager::GetCurrentScene();
    if (!activeScene || index < 0)
        return;

    std::string name = activeScene->GetMapObjects()[index].name;
    auto cmd = std::make_unique<DeleteObjectCommand>(activeScene, index);
    m_CommandHistory.PushCommand(std::move(cmd));
    CD_INFO("Deleted legacy object: %s", name.c_str());

    // Deselect if we deleted the current selection
    if (m_SelectionManager.GetSelectedIndex() == index)
        m_SelectionManager.SetSelection(-1, SelectionType::NONE);
}

void EditorLayer::CreateEntity()
{
    if (!m_Scene)
        return;

    // Temporary until circular dependency is resolved
    auto &registry = m_Scene->GetRegistry();
    entt::entity e = registry.create();
    registry.emplace<IDComponent>(e);
    registry.emplace<TagComponent>(e, "Empty Entity");
    registry.emplace<TransformComponent>(e);

    m_SelectionManager.SetEntitySelection(e);
    CD_INFO("Created new entity: %llu", (unsigned long long)registry.get<IDComponent>(e).ID);
}

void EditorLayer::DeleteEntity(entt::entity entity)
{
    if (!m_Scene || entity == entt::null)
        return;

    std::string tag = "Unknown";
    if (m_Scene->GetRegistry().all_of<TagComponent>(entity))
        tag = m_Scene->GetRegistry().get<TagComponent>(entity).Tag;

    m_Scene->DestroyEntity(entity);
    CD_INFO("Deleted entity: %s", tag.c_str());

    if (m_SelectionManager.GetSelectedEntity() == entity)
        m_SelectionManager.ClearSelection();
}

void EditorLayer::OnAssetDropped(const std::string &assetPath, const Vector3 &worldPosition)
{
    auto activeScene = MapManager::GetCurrentScene();
    if (!activeScene)
        return;

    std::filesystem::path fullPath(assetPath);
    std::string filename = fullPath.filename().string();
    std::string ext = fullPath.extension().string();

    // Check if it's a model
    if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
    {
        // 1. Ensure model is loaded in the scene
        auto &models = activeScene->GetMapModelsMutable();
        if (models.find(filename) == models.end())
        {
            Model model = LoadModel(assetPath.c_str());
            if (model.meshCount > 0)
            {
                models[filename] = model;
                CD_INFO("Loaded dropped model: %s", filename.c_str());
            }
            else
            {
                CD_ERROR("Failed to load dropped model: %s", assetPath.c_str());
                return;
            }
        }

        // 2. Create object instance
        MapObjectData obj;
        obj.name = filename;
        obj.type = MapObjectType::MODEL;
        obj.modelName = filename;
        obj.position = worldPosition;
        obj.scale = {1, 1, 1};
        obj.color = WHITE;

        AddObject(obj);
    }
}

// =========================================================================
// Getters & Setters
// =========================================================================

SceneState EditorLayer::GetSceneState() const
{
    return m_SimulationManager.GetSceneState();
}

SelectionManager &EditorLayer::GetSelectionManager()
{
    return m_SelectionManager;
}

Tool EditorLayer::GetActiveTool() const
{
    return m_ActiveTool;
}

void EditorLayer::SetActiveTool(Tool tool)
{
    m_ActiveTool = tool;
}

std::shared_ptr<Scene> EditorLayer::GetActiveScene()
{
    return m_Scene;
}

// =========================================================================
// Project Management
// =========================================================================

void EditorLayer::NewProject(const std::string &name, const std::string &location)
{
    auto activeProject = m_ProjectManager.NewProject(name, location);
    if (activeProject)
    {
        m_ShowProjectBrowser = false;
        m_ProjectBrowserPanel->AddRecentProject(activeProject->GetProjectFilePath().string());

        // Update asset browser to point to project assets
        if (m_AssetBrowserPanel)
        {
            m_AssetBrowserPanel->SetRootDirectory(activeProject->GetAssetDirectory());
        }

        CD_INFO("[EditorLayer] Created new project: %s", name.c_str());
    }
    else
    {
        CD_ERROR("[EditorLayer] Failed to create project: %s", name.c_str());
    }
}

void EditorLayer::OpenProject(const std::string &projectPath)
{
    auto activeProject = m_ProjectManager.OpenProject(projectPath);
    if (activeProject)
    {
        m_ShowProjectBrowser = false;
        m_ProjectBrowserPanel->AddRecentProject(projectPath);

        // Update asset browser
        if (m_AssetBrowserPanel)
        {
            m_AssetBrowserPanel->SetRootDirectory(activeProject->GetAssetDirectory());
        }

        CD_INFO("[EditorLayer] Opened project: %s", activeProject->GetName().c_str());
    }
    else
    {
        CD_ERROR("[EditorLayer] Failed to open project: %s", projectPath.c_str());
    }
}

void EditorLayer::CloseProject()
{
    m_ProjectManager.CloseProject();
    m_ShowProjectBrowser = true;
}

void EditorLayer::PlayInRuntime()
{
    if (!m_Scene)
    {
        CD_WARN("[EditorLayer] No active scene to play in runtime");
        return;
    }

    // Use project's scene directory if available, otherwise fallback to project root
    std::string tempScenePath;
    auto activeProject = m_ProjectManager.GetActiveProject();
    if (activeProject)
    {
        std::filesystem::path sceneDir = activeProject->GetSceneDirectory();
        if (!std::filesystem::exists(sceneDir))
        {
            std::filesystem::create_directories(sceneDir);
        }
        tempScenePath = (sceneDir / "RuntimeScene.chscene").string();
    }
    else
    {
        tempScenePath = (std::filesystem::path(PROJECT_ROOT_DIR) / "RuntimeScene.chscene").string();
    }

    CD_INFO("[EditorLayer] Saving runtime scene to: %s", tempScenePath.c_str());

    // Use ECSSceneSerializer to save the ECS scene
    CHEngine::ECSSceneSerializer serializer(m_Scene);
    serializer.Serialize(tempScenePath);

    CD_INFO("[EditorLayer] Scene saved successfully, launching runtime...");

    // Build runtime executable path
    std::string runtimePath = std::string(PROJECT_ROOT_DIR) + "/build/bin/Runtime.exe";

    // Build command line with --map and --skip-menu arguments
    std::string commandLine = "\"" + runtimePath + "\" --map \"" + tempScenePath + "\" --skip-menu";

    CD_INFO("[EditorLayer] Launching: %s", commandLine.c_str());

    // Launch runtime process using ProcessUtils (non-blocking)
    if (ProcessUtils::LaunchProcess(commandLine, PROJECT_ROOT_DIR))
    {
        CD_INFO("[EditorLayer] Runtime launched successfully");
    }
    else
    {
        CD_ERROR("[EditorLayer] Failed to launch runtime");
    }
}

} // namespace CHEngine
