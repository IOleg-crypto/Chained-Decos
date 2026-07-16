#include "layer.h"
#include "editor_colors.h"
#include "engine/core/input.h"
#include "engine/core/service_locator.h"
#include "engine/imgui/imgui_layer.h"
#include "events.h"
#include "gui.h"
#include "layout.h"
#include "panels.h"

#include "engine/app/application.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/profiler.h"
#include "engine/common/thread_pool.h"
#include "engine/graphics/api/graphics_device.h"
#include "engine/graphics/ui/ui_renderer.h"
#include "engine/physics/physics.h"
#include "engine/project/project.h"
#include "panels/property_editor.h"
#include "panels/viewport_panel.h"
#include "scripting/scriptengine.h"
#include "thirdparty/IconsFontAwesome6.h"
#include "ui/project_selector_ui.h"

#include <ImGuizmo.h>
#include <yaml-cpp/yaml.h>

namespace Chained
{
void EditorLayer::DrawLoadingOverlay(const char* title, const char* status)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                             ImGuiWindowFlags_NoInputs;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, EditorColors::LoadingOverlayBg);

    if (ImGui::Begin("##EditorLoadingOverlay", nullptr, flags))
    {
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.45f);

        ImVec2 titleSize = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
        ImGui::TextUnformatted(title);

        ImVec2 statusSize = ImGui::CalcTextSize(status);
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - statusSize.x) * 0.5f);
        ImGui::TextUnformatted(status);

        uint32_t totalPending = (uint32_t)ServiceLocator::Get<AssetManager>()->GetPendingFinalizeCount();
        char pendingBuffer[64];
        snprintf(pendingBuffer, sizeof(pendingBuffer), "Pending assets: %u", totalPending);

        ImVec2 pendingSize = ImGui::CalcTextSize(pendingBuffer);
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - pendingSize.x) * 0.5f);
        ImGui::TextUnformatted(pendingBuffer);
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

EditorLayer::EditorLayer()
    : Layer("EditorLayer")
{
    s_Instance = this;

    // Safe here: Application's constructor already calls ServiceLocator::Lock() before
    // any layer is constructed (layers are pushed from CreateApplication after `new
    // Application(spec)` returns). Resolved once, reused for the layer's whole lifetime.
    m_Context.PhysicsSystem = ServiceLocator::Get<Physics>();
    m_Context.Scripting = ServiceLocator::Get<ScriptEngine>();
    m_Context.UI = ServiceLocator::TryGet<UIRenderer>(); // null in headless mode

    // Default debug settings
    GetDebugRenderFlags().DrawColliders = true;
    GetDebugRenderFlags().DrawLights = true;
    GetDebugRenderFlags().DrawSpawnZones = true;

    m_ProjectManager = std::make_unique<EditorProjectManager>();
    m_SceneManager = std::make_unique<EditorSceneManager>(
        m_CommandHistory, *m_ProjectManager, m_Config, m_ViewportSize, m_EditorState, m_Context);

    // Forward scene events (e.g. SceneChangeRequestEvent) back to EditorLayer::OnEvent
    m_SceneManager->SetSceneEventCallback([this](Event& e) { OnEvent(e); });

    m_Panels = std::make_unique<EditorPanels>(*this);

    m_Layout = std::make_unique<EditorLayout>(*m_Panels);
    m_ProjectSelectorUI = std::make_unique<ProjectSelectorUI>(*m_ProjectManager);

    LoadConfig();
}

EditorLayer::~EditorLayer()
{
    SetSelectedEntity({});
    s_Instance = nullptr;
}

void EditorLayer::LoadConfig()
{
    std::filesystem::path configPath = std::filesystem::current_path() / "editor_settings.yaml";
    if (!std::filesystem::exists(configPath))
    {
        return;
    }

    try
    {
        YAML::Node data = YAML::LoadFile(configPath.string());
        if (data["Editor"])
        {
            auto node = data["Editor"];
            if (node["LastProjectPath"])
            {
                std::string lastProj = node["LastProjectPath"].as<std::string>("");
                m_ProjectManager->SetLastProjectPath(lastProj);
                m_Config.LastProjectPath = lastProj;
            }
            if (node["LastScenePath"])
            {
                m_Config.LastScenePath = node["LastScenePath"].as<std::string>("");
            }
            if (node["LoadLastProjectOnStartup"])
            {
                m_Config.LoadLastProjectOnStartup = node["LoadLastProjectOnStartup"].as<bool>(false);
            }
            if (node["AutoSaveEnabled"])
            {
                m_Config.AutoSaveEnabled = node["AutoSaveEnabled"].as<bool>(true);
            }
            if (node["AutoSaveInterval"])
            {
                m_Config.AutoSaveInterval = node["AutoSaveInterval"].as<float>(300.0f);
            }
            if (node["RecentProjects"])
            {
                m_Config.RecentProjects.clear();
                for (const auto& entry : node["RecentProjects"])
                {
                    m_Config.RecentProjects.push_back(entry.as<std::string>());
                }
            }
        }
    } catch (const std::exception& e)
    {
        CH_CORE_ERROR("EditorLayer: Failed to load editor settings: {}", e.what());
    }
}

void EditorLayer::SaveConfig()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Editor" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "LastProjectPath" << YAML::Value << m_ProjectManager->GetLastProjectPath();
    m_Config.LastProjectPath = m_ProjectManager->GetLastProjectPath();
    out << YAML::Key << "LastScenePath" << YAML::Value << m_Config.LastScenePath;
    out << YAML::Key << "LoadLastProjectOnStartup" << YAML::Value << m_Config.LoadLastProjectOnStartup;
    out << YAML::Key << "AutoSaveEnabled" << YAML::Value << m_Config.AutoSaveEnabled;
    out << YAML::Key << "AutoSaveInterval" << YAML::Value << m_Config.AutoSaveInterval;

    out << YAML::Key << "RecentProjects" << YAML::Value << YAML::BeginSeq;
    for (const auto& path : m_Config.RecentProjects)
    {
        out << path;
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;
    out << YAML::EndMap;

    std::filesystem::path configPath = std::filesystem::current_path() / "editor_settings.yaml";
    std::ofstream fout(configPath);
    fout << out.c_str();
}

void EditorLayer::OnAttach()
{
    // Ensure this module (EXE) uses the same ImGui context as the engine DLL
    auto& app = Application::Get();
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(app.GetImGuiLayer()->GetContext()));

    // SetTraceLogCallback removed - now using engine logging

    EditorGUI::ApplyTheme();
    PropertyEditor::Init();
    m_Panels->Init();

    m_CommandHistory.SetNotifyCallback(
        []() { CH_CORE_TRACE("CommandHistory: Scene state changed, notifying editor..."); });

    // Auto-load last project/scene
    const auto& config = GetConfig();

    if (config.LoadLastProjectOnStartup && !m_ProjectManager->GetLastProjectPath().empty() &&
        std::filesystem::exists(m_ProjectManager->GetLastProjectPath()))
    {
        CH_CORE_INFO("Auto-loading last project: {}", m_ProjectManager->GetLastProjectPath());
        m_ProjectManager->OpenProject(m_ProjectManager->GetLastProjectPath());

        if (!config.LastScenePath.empty() && std::filesystem::exists(config.LastScenePath))
        {
            CH_CORE_INFO("Auto-loading last scene: {}", config.LastScenePath);
            m_SceneManager->OpenScene(config.LastScenePath);
        }
    }
    else
    {
        Project::SetActive(nullptr);
    }

    // Ensure layout is initialized
    const char* iniPath = ImGui::GetIO().IniFilename;
    if (iniPath && !std::filesystem::exists(iniPath))
    {
        CH_CORE_INFO("OnAttach: Layout file '{}' not found, will be reset on first frame", iniPath);
        GetEditorState().NeedsLayoutReset = true;
    }

    std::string iconPath =
        (ServiceLocator::Get<AssetManager>()->GetEngineRoot() / "resources/icons/chaineddecosmapeditor.jpg").string();
    if (std::filesystem::exists(iconPath))
    {
        app.GetWindow().SetWindowIcon(iconPath);
    }
    else
    {
        CH_CORE_WARN("Editor icon not found at: {}", iconPath);
    }
    CH_CORE_INFO("EditorLayer Attached with modular panels.");

    LoadEditorFonts();
}

void EditorLayer::LoadEditorFonts()
{
    auto* imguiLayer = Application::Get().GetImGuiLayer();
    if (!imguiLayer)
    {
        CH_CORE_ERROR("EditorLayer: ImGuiLayer not found!");
        return;
    }

    float fontSize = 16.0f;
    auto& assetManager = (*ServiceLocator::Get<AssetManager>());

    // --- Default UI Font (Lato) ---
    std::string fontPath =
        (ServiceLocator::Get<AssetManager>()->GetEngineRoot() / "resources/font/lato/lato-bold.ttf").string();
    if (std::filesystem::exists(fontPath))
    {
        imguiLayer->AddFontFromFile(fontPath, fontSize);
        CH_CORE_INFO("Loaded editor font: {}", fontPath);
    }
    else
    {
        CH_CORE_WARN("Editor font not found: {}. Using default ImGui font.", fontPath);
        ImGui::GetIO().Fonts->AddFontDefault();
    }

    // --- Icon Font (FontAwesome) ---
    std::string faPath =
        (ServiceLocator::Get<AssetManager>()->GetEngineRoot() / "resources/font/fa-solid-900.ttf").string();
    if (std::filesystem::exists(faPath))
    {
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        imguiLayer->AddFontFromFile(faPath, fontSize, &icons_config, icons_ranges);
        CH_CORE_INFO("Loaded and merged FontAwesome for editor: {}", faPath);
    }

    imguiLayer->RefreshFontAtlasTexture();
}

void EditorLayer::OnDetach()
{
    if (auto scene = GetActiveScene())
    {
        if (scene->GetSceneState() != SceneState::Edit)
        {
            scene->OnRuntimeStop(m_Context);
        }
    }
    SaveConfig();
}

void EditorLayer::OnUpdate(Timestep ts)
{
    CH_PROFILE_FUNCTION();

    if (!m_PendingSceneTransitionPath.empty())
    {
        m_SceneManager->OpenScene(m_PendingSceneTransitionPath);
        
        // Ensure play mode continues after scene load
        m_SceneManager->SetSceneState(SceneState::Play);
        
        m_PendingSceneTransitionPath.clear();
    }

    // 1. Scene manager updates internal transitions and the current active scene
    m_SceneManager->OnUpdate(ts);

    // 2. Update panels
    m_Panels->SetContext(GetActiveScene());
    m_Panels->OnUpdate(ts);

    // 3. If loading — skip everything else
    if (m_SceneManager->IsLoading())
    {
        return;
    }

    // 4. Logic update (Play/Simulate/Edit) is now handled by the scene
    // We just call OnUpdate on the active scene
    if (auto scene = GetActiveScene())
    {
        // If scene is in Play mode, ask ScriptEngine to execute scripts
        if (scene->GetSceneState() == SceneState::Play)
        {
            auto& scriptEngine = *ServiceLocator::Get<ScriptEngine>();
            if (scriptEngine.GetHost().IsInitialized() && scriptEngine.CanExecuteFrameScripts())
            {
                scene->OnUpdateRuntime(ts, m_Context);
            }
        }
        else if (scene->GetSceneState() == SceneState::Simulate)
        {
            scene->OnUpdateSimulation(ts, m_Context);
        }
        else
        {
            scene->OnUpdateEditor(ts, m_Context);

            if (m_Config.AutoSaveEnabled)
            {
                m_SceneManager->AutoSave(m_Config.AutoSaveInterval, ts);
            }
        }
    }

    // Input shortcuts
    if (Core::Input::IsKeyPressed(KeyCode::F5))
    {
        AppLaunchRuntimeEvent e;
        OnEvent(e);
    }
}

void EditorLayer::OnRender(Timestep ts)
{
    GraphicsDevice::Get().Clear({25, 25, 25, 255});
}

void EditorLayer::OnImGuiRender()
{
    // ImGuizmo context sync only — BeginFrame() is already called in ImGuiLayer::Begin()
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

    if (GetEditorState().NeedsLayoutReset)
    {
        ResetLayout();
        GetEditorState().NeedsLayoutReset = false;
    }

    bool hasProject = Project::GetActive() != nullptr;

    if (hasProject && GetEditorState().FullscreenGame)
    {
        if (auto viewportPanel = m_Panels->Get<ViewportPanel>())
        {
            viewportPanel->OnImGuiRender(true);
        }
    }
    else if (hasProject)
    {
        m_Layout->OnImGuiRender();
    }
    else
    {
        m_ProjectSelectorUI->OnImGuiRender();
    }

    if (m_SceneManager->IsLoading())
    {
        DrawLoadingOverlay("Editor Busy", m_SceneManager->GetLoadingStatus().c_str());
    }
}

void EditorLayer::ResetLayout()
{
    m_Layout->ResetLayout();
}

// Project and Scene event handlers are now managed by EditorProjectManager and EditorSceneManager.

std::shared_ptr<Scene> EditorLayer::GetActiveScene() const
{
    return m_SceneManager->GetActiveScene();
}

void EditorLayer::OnEvent(Event& e)
{
    if (auto scene = GetActiveScene())
    {
        scene->OnEvent(e);
    }

    // Dispatch events to all editor panels
    m_Panels->OnEvent(e);

    EventDispatcher dispatcher(e);

    // 1. Scene Management
    dispatcher.Dispatch<SceneOpenedEvent>([this](auto& e) { return m_SceneManager->OnSceneOpened(e); });
    dispatcher.Dispatch<ScenePlayEvent>([this](auto& e) {
        m_SceneManager->SetSceneState(SceneState::Play);
        return true;
    });
    dispatcher.Dispatch<SceneSimulateEvent>([this](auto& e) {
        m_SceneManager->SetSceneState(SceneState::Simulate);
        return true;
    });
    dispatcher.Dispatch<SceneStopEvent>([this](auto& e) {
        m_SceneManager->SetSceneState(SceneState::Edit);
        return true;
    });

    // 2. Project Management
    dispatcher.Dispatch<ProjectOpenedEvent>([this](auto& e) { return m_ProjectManager->OnProjectOpened(e); });
    dispatcher.Dispatch<AppLaunchRuntimeEvent>([this](auto& e) {
        // Enter play mode first if not already
        if (GetSceneState() != SceneState::Play)
        {
            m_SceneManager->SetSceneState(SceneState::Play);
        }
        else
        {
            m_SceneManager->SetSceneState(SceneState::Edit);
        }
        return true;
    });

    // 3. Command/Undo
    dispatcher.Dispatch<UndoEvent>([this](auto& e) {
        if (GetSceneState() != SceneState::Play && GetSceneState() != SceneState::Simulate)
        {
            m_CommandHistory.Undo();
        }
        return true;
    });

    dispatcher.Dispatch<RedoEvent>([this](auto& e) {
        if (GetSceneState() != SceneState::Play && GetSceneState() != SceneState::Simulate)
        {
            m_CommandHistory.Redo();
        }
        return true;
    });

    // 4. Input
    dispatcher.Dispatch<KeyPressedEvent>([this](auto& e) { return m_SceneManager->OnKeyPressed(e); });
    // 3. Layout/System
    dispatcher.Dispatch<AppResetLayoutEvent>([this](auto& ev) {
        ResetLayout();
        return true;
    });
    dispatcher.Dispatch<AppSaveLayoutEvent>([this](auto& ev) {
        m_Layout->SaveDefaultLayout();
        return true;
    });
    dispatcher.Dispatch<SceneChangeRequestEvent>([this](auto& ev) {
        std::filesystem::path scenePath = ev.GetPath();
        // If the path is relative, resolve it via Project::GetAssetPath
        if (scenePath.is_relative() && Project::GetActive())
        {
            scenePath = Project::GetAssetPath(ev.GetPath());
        }

        std::string finalPath = scenePath.string();

        m_PendingSceneTransitionPath = finalPath;
        return true;
    });

    // 4. Selections/Picking
    dispatcher.Dispatch<EntitySelectedEvent>([this](auto& ev) {
        SetSelectedEntity(Entity(ev.GetEntity(), &ev.GetScene()->GetRegistry()));
        GetEditorState().LastHitMeshIndex = ev.GetMeshIndex();
        return false;
    });

    // 5. Raw Input Overrides
    if (e.GetEventType() == EventType::KeyPressed)
    {
        auto& ke = (KeyPressedEvent&)e;
        if (ke.GetKeyCode() == KeyCode::Escape && GetEditorState().FullscreenGame)
        {
            GetEditorState().FullscreenGame = false;
            e.Handled = true;
        }
        else if (ke.GetKeyCode() == KeyCode::F11)
        {
            Application::Get().GetWindow().ToggleFullscreen();
            e.Handled = true;
        }
    }
}

CommandHistory& EditorLayer::GetCommandHistory()
{
    return m_CommandHistory;
}


void EditorLayer::ReparentEntity(Entity child, Entity parent)
{
    if (child.HasComponent<HierarchyComponent>())
    {
        child.GetComponent<HierarchyComponent>().Parent = parent;
    }
}

} // namespace Chained
