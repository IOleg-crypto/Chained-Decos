#include "editor_layer.h"
#include "editor_panels.h"
#include "editor_layout.h"
#include "editor_events.h"
#include "editor_gui.h"
#include "engine/core/imgui_layer.h"
#include "engine/core/input.h"
#include "launcher/editor_launcher.h"

#include "IconsFontAwesome6.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/profiler.h"
#include "engine/core/thread_pool.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/pipeline/ui_renderer.h"
#include "engine/physics/physics.h"
#include "engine/platform/utils/dialogs.h"
#include "engine/scene/project.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene_serializer.h"
#include "panels/console_panel.h"
#include "panels/content_browser_panel.h"
#include "panels/project_browser_panel.h"
#include "panels/property_editor.h"
#include "panels/viewport_panel.h"
#include "scripting/scene_scripting.h"
#include "scripting/scriptengine.h"
#include <ImGuizmo.h>
#include <chrono>
#include <yaml-cpp/yaml.h>


namespace CHEngine
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
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.02f, 0.02f, 0.92f));

    if (ImGui::Begin("##EditorLoadingOverlay", nullptr, flags))
    {
        const size_t loadingCount = CHEngine::AssetManager::Get().GetLoadingAssetCount();
        const size_t pendingFinalizeCount = CHEngine::AssetManager::Get().GetPendingFinalizeCount();
        const size_t totalPending = loadingCount + pendingFinalizeCount;

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.45f);

        ImVec2 titleSize = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
        ImGui::TextUnformatted(title);

        ImVec2 statusSize = ImGui::CalcTextSize(status);
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - statusSize.x) * 0.5f);
        ImGui::TextUnformatted(status);

        std::string pendingLine = "Pending assets: " + std::to_string(totalPending);
        ImVec2 pendingSize = ImGui::CalcTextSize(pendingLine.c_str());
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - pendingSize.x) * 0.5f);
        ImGui::TextUnformatted(pendingLine.c_str());
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

EditorLayer* EditorLayer::s_Instance = nullptr;

EditorLayer::EditorLayer()
    : Layer("EditorLayer")
{
    // Ensure the engine DLL uses the same ImGui context as the Editor
    ImGuiLayer::SetContext(ImGui::GetCurrentContext());

    s_Instance = this;
    EditorContext::Init();

    m_ProjectManager = std::make_unique<EditorProjectManager>();
    m_SceneManager = std::make_unique<EditorSceneManager>();
    m_Layout = std::make_unique<EditorLayout>();
    m_Panels = std::make_unique<EditorPanels>();

    LoadConfig();
}

EditorLayer::~EditorLayer()
{
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
    // SetTraceLogCallback removed - now using engine logging

    EditorGUI::ApplyTheme();
    Log::SetLogCallback(ConsolePanel::AddLog);
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
        EditorContext::GetState().NeedsLayoutReset = true;
    }

    std::string iconPath = AssetManager::Get().ResolvePath("engine/resources/icons/chaineddecosmapeditor.jpg");
    if (std::filesystem::exists(iconPath))
    {
        Application::Get().GetWindow().SetWindowIcon(iconPath);
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

    ImGuiIO& io = ImGui::GetIO();
    float fontSize = 16.0f;
    auto& assetManager = AssetManager::Get();

    // --- Default UI Font (Lato) ---
    std::string fontPath = assetManager.ResolvePath("engine/resources/font/lato/lato-bold.ttf");
    if (std::filesystem::exists(fontPath))
    {
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
        CH_CORE_INFO("Loaded editor font: {}", fontPath);
    }
    else
    {
        CH_CORE_WARN("Editor font not found: {}. Using default ImGui font.", fontPath);
        io.Fonts->AddFontDefault();
    }

    // --- Icon Font (FontAwesome) ---
    std::string faPath = assetManager.ResolvePath("engine/resources/font/fa-solid-900.ttf");
    if (std::filesystem::exists(faPath))
    {
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        io.Fonts->AddFontFromFileTTF(faPath.c_str(), fontSize, &icons_config, icons_ranges);
        CH_CORE_INFO("Loaded and merged FontAwesome for editor: {}", faPath);
    }

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
}

void EditorLayer::OnDetach()
{
    SaveConfig();
    EditorContext::Shutdown();
}

void EditorLayer::OnUpdate(Timestep ts)
{
    CH_PROFILE_FUNCTION();

    m_SceneManager->OnUpdate(ts);

    // Sync context to panels
    m_Panels->SetContext(GetActiveScene());

    // Update all panels (includes viewport camera controller)
    m_Panels->OnUpdate(ts);

    if (auto scene = GetActiveScene())
    {
        if (EditorContext::GetSceneState() == SceneState::Play)
        {
            auto& scriptEngine = ScriptEngine::Get();

            if (scriptEngine.CanExecuteFrameScripts())
            {
                SceneScripting::Update(scene.get(), ts);
            }
            scene->OnUpdateRuntime(ts);

            // Handle deferred scene loading requested from C# scripts
            std::string pendingPath;
            if (scriptEngine.TryConsumeRequestedScene(pendingPath))
            {
                SceneChangeRequestEvent ev(pendingPath);
                OnEvent(ev);
            }
        }
        else
        {
            scene->OnUpdateEditor(ts);

            // Auto-save logic (delegated to SceneManager)
            if (m_Config.AutoSaveEnabled)
            {
                m_SceneManager->AutoSave(m_Config.AutoSaveInterval, ts);
            }
        }

        if (Input::IsKeyPressed(Key::F5))
        {
            AppLaunchRuntimeEvent e;
            OnEvent(e);
        }

        if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed(Key::R))
        {
            auto& scriptEngine = ScriptEngine::Get();
            scriptEngine.RequestAssemblyReload("EditorLayer");
        }
    }
}

void EditorLayer::OnRender(Timestep ts)
{
    RenderCommand::Clear({25, 25, 25, 255});
}

void EditorLayer::OnImGuiRender()
{
    ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
    ImGuizmo::BeginFrame();

    if (EditorContext::GetState().NeedsLayoutReset)
    {
        ResetLayout();
        EditorContext::GetState().NeedsLayoutReset = false;
    }

    if (Project::GetActive())
    {
        if (EditorContext::GetState().FullscreenGame)
        {
            if (auto viewportPanel = m_Panels->Get<ViewportPanel>())
            {
                viewportPanel->OnImGuiRender(true);
            }
        }
        else
        {
            DrawDockSpace();
        }
    }
    else
    {
        if (auto projectBrowser = m_Panels->Get<ProjectBrowserPanel>())
        {
            projectBrowser->OnImGuiRender();
        }
    }

    if (EditorContext::GetState().IsLoading)
    {
        DrawLoadingOverlay("Editor Busy", EditorContext::GetState().LoadingStatus.c_str());
    }
}

void EditorLayer::ResetLayout()
{
    m_Layout->ResetLayout();
}

void EditorLayer::DrawDockSpace()
{
    m_Layout->BeginWorkspace();
    m_Layout->DrawInterface();
    m_Layout->EndWorkspace();
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
        SceneScripting::DispatchEvent(scene.get(), e);
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
    dispatcher.Dispatch<SceneStopEvent>([this](auto& e) {
        m_SceneManager->SetSceneState(SceneState::Edit);
        return true;
    });

    // 2. Project Management
    dispatcher.Dispatch<ProjectOpenedEvent>([this](auto& e) { return m_ProjectManager->OnProjectOpened(e); });
    dispatcher.Dispatch<AppLaunchRuntimeEvent>([this](auto& e) {
        LaunchStandalone();
        return true;
    });

    // 3. Command/Undo
    dispatcher.Dispatch<UndoEvent>([this](auto& e) {
        m_CommandHistory.Undo();
        return true;
    });
    dispatcher.Dispatch<RedoEvent>([this](auto& e) {
        m_CommandHistory.Redo();
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

        if (EditorContext::GetSceneState() == SceneState::Play)
        {
            // Handle mid-play scene change directly or via Manager
            m_SceneManager->OpenScene(finalPath);
            return true;
        }
        m_SceneManager->OpenScene(finalPath);
        return true;
    });

    // 4. Selections/Picking
    dispatcher.Dispatch<EntitySelectedEvent>([this](auto& ev) {
        EditorContext::SetSelectedEntity(Entity(ev.GetEntity(), &ev.GetScene()->GetRegistry()));
        EditorContext::GetState().LastHitMeshIndex = ev.GetMeshIndex();
        return false;
    });

    // 6. Raw Input Overrides
    if (EditorContext::GetSceneState() == SceneState::Play)
    {
        // Script events are already dispatched on line 390
    }
    else if (e.GetEventType() == EventType::KeyPressed)
    {
        auto& ke = (KeyPressedEvent&)e;
        if (ke.GetKeyCode() == Key::Escape && EditorContext::GetState().FullscreenGame)
        {
            EditorContext::GetState().FullscreenGame = false;
            e.Handled = true;
        }
    }
}

CommandHistory& EditorLayer::GetCommandHistory()
{
    return s_Instance->m_CommandHistory;
}

// File and project operations are now handled by EditorProjectManager.

void EditorLayer::LaunchStandalone()
{
    CH_PROFILE_FUNCTION();
    EditorLauncher::LaunchStandalone(Project::GetActive(), GetActiveScene());
}

void EditorLayer::ReparentEntity(Entity child, Entity parent)
{
    if (child.HasComponent<HierarchyComponent>())
    {
        child.GetComponent<HierarchyComponent>().Parent = parent;
    }
}

} // namespace CHEngine
