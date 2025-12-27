#include "EditorLayer.h"
#include "core/Base.h"
#include "core/Engine.h"
#include "core/Log.h"
#include "core/application/EngineApplication.h"
#include "core/input/Input.h"
#include "core/renderer/Renderer.h"
#include "editor/utils/EditorStyles.h"
#include "scene/ecs/components/TransformComponent.h"
#include "scene/ecs/components/core/IDComponent.h"
#include "scene/ecs/components/core/TagComponent.h"
#include <cstdlib>

#include "core/interfaces/ILevelManager.h"
#include "core/interfaces/IPlayer.h"
#include "editor/logic/SceneCloner.h"
#include "editor/logic/undo/AddObjectCommand.h"
#include "editor/logic/undo/DeleteObjectCommand.h"
#include "editor/logic/undo/ModifyObjectCommand.h"
#include "editor/logic/undo/TransformCommand.h"
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

    // Initialize new Scene system
    m_Scene = std::make_shared<Scene>("EditorScene");
    CD_INFO("[EditorLayer] Created editor scene: %s", m_Scene->GetName().c_str());

    // Create default entities using raw entt::entity (temporary workaround for circular dependency)
    auto &registry = m_Scene->GetRegistry();

    // Ground entity
    entt::entity ground = registry.create();
    registry.emplace<IDComponent>(ground);
    registry.emplace<TagComponent>(ground, "Ground");
    registry.emplace<TransformComponent>(ground);

    // Cube entity
    entt::entity cube = registry.create();
    registry.emplace<IDComponent>(cube);
    registry.emplace<TagComponent>(cube, "Default Cube");
    auto &cubeTransform = registry.emplace<TransformComponent>(cube);
    cubeTransform.position = {0, 0.5f, 0};

    CD_INFO("[EditorLayer] Created 2 default entities in scene");

    // Legacy GameScene initialization
    m_ProjectManager.SetSceneChangedCallback(
        [this](std::shared_ptr<GameScene> scene)
        {
            m_ActiveScene = scene;
            m_EditorScene = scene;
            if (m_HierarchyPanel)
                m_HierarchyPanel->SetContext(scene);
        });

    m_ActiveScene = m_ProjectManager.GetActiveScene();
    m_EditorScene = m_ActiveScene;

    m_HierarchyPanel = std::make_unique<HierarchyPanel>(m_ActiveScene);
    m_HierarchyPanel->SetSceneContext(m_Scene); // Connect to new Scene system
    m_InspectorPanel = std::make_unique<InspectorPanel>();
    m_ViewportPanel = std::make_unique<ViewportPanel>();
    m_AssetBrowserPanel = std::make_unique<AssetBrowserPanel>();
    m_ToolbarPanel = std::make_unique<ToolbarPanel>();
    m_MenuBarPanel = std::make_unique<MenuBarPanel>();
    m_ConsolePanel = std::make_unique<ConsolePanel>();

    m_InspectorPanel->SetSkyboxCallback([this](const std::string &path) { LoadSkybox(path); });

    // Default Scene Content (legacy)
    m_ActiveScene = m_EditorScene;
    auto &objects = m_ActiveScene->GetMapObjectsMutable();

    // ground
    MapObjectData ground_legacy;
    ground_legacy.name = "Ground";
    ground_legacy.type = MapObjectType::PLANE;
    ground_legacy.size = {100, 100};
    ground_legacy.color = DARKGRAY;
    objects.push_back(ground_legacy);

    // default cube
    MapObjectData cube_legacy;
    cube_legacy.name = "Default Cube";
    cube_legacy.type = MapObjectType::CUBE;
    cube_legacy.position = {0, 0.5f, 0};
    cube_legacy.color = WHITE;
    objects.push_back(cube_legacy);

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
    if (m_SimulationManager.GetSceneState() == SceneState::Play)
    {
        // Update Game Logic via Engine modules
        auto levelManager = Engine::Instance().GetService<ILevelManager>();
        if (levelManager)
        {
            levelManager->Update(deltaTime);
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

    // Direct simulation escape check
    if (m_SimulationManager.GetSceneState() == SceneState::Play &&
        Input::IsKeyPressed(KEY_BACKSPACE))
    {
        OnSceneStop();
    }
}

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

    // Panels
    if (m_HierarchyPanel && m_HierarchyPanel->IsVisible())
    {
        m_HierarchyPanel->OnImGuiRender(
            m_SelectionManager.GetSelectionType(), m_SelectionManager.GetSelectedIndex(),
            [this](SelectionType type, int i) { m_SelectionManager.SetSelection(i, type); },
            [this]() { AddModel(); }, [this](const std::string &type) { AddUIElement(type); },
            [this](int i) { DeleteObject(i); });

        m_InspectorPanel->SetPropertyChangeCallback(
            [this](int index, const MapObjectData &oldData, const MapObjectData &newData)
            {
                // If index is -1, use currently selected object index
                int actualIndex =
                    (index == -1) ? m_SelectionManager.GetSelectedObjectIndex() : index;

                if (actualIndex != -1)
                {
                    // Ensure texture is loaded if it changed
                    if (newData.texturePath != oldData.texturePath && !newData.texturePath.empty())
                    {
                        auto &textures = m_ActiveScene->GetMapTexturesMutable();
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
                        m_ActiveScene, actualIndex, oldData, newData));
                }
            });
    }

    if (m_InspectorPanel && m_InspectorPanel->IsVisible())
    {
        if (m_SelectionManager.GetSelectionType() == SelectionType::UI_ELEMENT)
            m_InspectorPanel->OnImGuiRender(m_ActiveScene, GetSelectedUIElement());
        else
            m_InspectorPanel->OnImGuiRender(
                m_ActiveScene, m_SelectionManager.GetSelectedObjectIndex(), GetSelectedObject());
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
            m_ActiveScene, activeCamera, m_SelectionManager.GetSelectedIndex(), m_ActiveTool,
            [this](int i) { m_SelectionManager.SetSelection(i, SelectionType::WORLD_OBJECT); },
            &m_CommandHistory,
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
            if (m_SelectionManager.GetSelectedIndex() >= 0 && m_ActiveScene)
            {
                auto &objects = m_ActiveScene->GetMapObjectsMutable();
                if (m_SelectionManager.GetSelectedIndex() < (int)objects.size())
                {
                    objects.erase(objects.begin() + m_SelectionManager.GetSelectedIndex());
                    m_SelectionManager.ClearSelection();
                }
            }
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
    callbacks.OnNew = [this]() { NewScene(); };
    callbacks.OnOpen = [this]() { OpenScene(); };
    callbacks.OnSave = [this]() { SaveScene(); };
    callbacks.OnSaveAs = [this]() { SaveSceneAs(); };
    callbacks.OnExit = []() { Engine::Instance().RequestExit(); };

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

void EditorLayer::OnScenePlay()
{
    m_SimulationManager.OnScenePlay(m_ActiveScene, m_EditorScene,
                                    m_SimulationManager.GetRuntimeMode(), &m_RuntimeLayer,
                                    GetAppRunner());
}

void EditorLayer::OnSceneStop()
{
    m_SimulationManager.OnSceneStop(m_ActiveScene, m_EditorScene, &m_RuntimeLayer, GetAppRunner());
}

MapObjectData *EditorLayer::GetSelectedObject()
{
    return m_SelectionManager.GetSelectedObject(m_ActiveScene);
}

UIElementData *EditorLayer::GetSelectedUIElement()
{
    return m_SelectionManager.GetSelectedUIElement(m_ActiveScene);
}

void EditorLayer::NewScene()
{
    m_ProjectManager.NewScene();
}

void EditorLayer::OpenScene()
{
    m_ProjectManager.OpenScene();
}

void EditorLayer::SaveScene()
{
    m_ProjectManager.SaveScene();
}

void EditorLayer::SaveSceneAs()
{
    m_ProjectManager.SaveSceneAs();
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

        MapObjectData obj;
        obj.name = filename;
        obj.type = MapObjectType::MODEL;
        obj.modelName = filename;
        obj.position = {0, 0, 0};
        obj.scale = {1, 1, 1};
        obj.color = WHITE;

        auto &objects = m_ActiveScene->GetMapObjectsMutable();
        objects.push_back(obj);
        m_SelectionManager.SetSelection((int)objects.size() - 1, SelectionType::WORLD_OBJECT);

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
    m_SelectionManager.SetSelection((int)elements.size() - 1, SelectionType::UI_ELEMENT);
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

void EditorLayer::AddObject(const MapObjectData &data)
{
    if (!m_ActiveScene)
        return;

    auto cmd = std::make_unique<AddObjectCommand>(m_ActiveScene, data);
    m_CommandHistory.PushCommand(std::move(cmd));
}

void EditorLayer::DeleteObject(int index)
{
    if (!m_ActiveScene || index < 0)
        return;

    auto cmd = std::make_unique<DeleteObjectCommand>(m_ActiveScene, index);
    m_CommandHistory.PushCommand(std::move(cmd));

    // Deselect if we deleted the current selection
    if (m_SelectionManager.GetSelectedIndex() == index)
        m_SelectionManager.SetSelection(-1, SelectionType::NONE);
}

void EditorLayer::OnAssetDropped(const std::string &assetPath, const Vector3 &worldPosition)
{
    if (!m_ActiveScene)
        return;

    std::filesystem::path fullPath(assetPath);
    std::string filename = fullPath.filename().string();
    std::string ext = fullPath.extension().string();

    // Check if it's a model
    if (ext == ".obj" || ext == ".glb" || ext == ".gltf")
    {
        // 1. Ensure model is loaded in the scene
        auto &models = m_ActiveScene->GetMapModelsMutable();
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

} // namespace CHEngine
