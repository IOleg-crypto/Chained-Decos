#include "editor_layer.h"
#include "editor/actions/project_actions.h"
#include "editor/actions/scene_actions.h"
#include "editor/editor.h"
#include "editor_gui.h"
#include "editor_events.h"
#include "engine/core/input.h"

#include "engine/core/profiler.h"
#include "engine/physics/physics.h"
#include "engine/scene/components.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/scriptable_entity.h"
#include "engine/graphics/asset_manager.h"
// Forward declaration: statically linked game scripts (defined in game_module.cpp)
extern void RegisterGameScripts(CHEngine::Scene* scene);

#include "cstdarg"
#include "extras/IconsFontAwesome6.h"
#include "nfd.h"
#include "panels/console_panel.h"
#include "panels/content_browser_panel.h"
#include "panels/environment_panel.h"
#include "panels/project_browser_panel.h"
#include "panels/viewport_panel.h"
#include "panels/property_editor.h"
#include "raylib.h"

namespace CHEngine
{
    EditorLayer *EditorLayer::s_Instance = nullptr;

    EditorLayer::EditorLayer() : Layer("EditorLayer")
    {
        s_Instance = this;
        EditorContext::Init();
        m_Panels = std::make_unique<EditorPanels>();
        m_Layout = std::make_unique<EditorLayout>();
        m_Actions = std::make_unique<EditorActions>();
    }

    void EditorLayer::OnAttach()
    {
        SetTraceLogCallback(
            [](int logLevel, const char *text, va_list args)
            {
                char buffer[4096];
                vsnprintf(buffer, sizeof(buffer), text, args);
                printf("%s\n", buffer);

                ConsoleLogLevel level = ConsoleLogLevel::Info;
                if (logLevel == LOG_WARNING) level = ConsoleLogLevel::Warn;
                else if (logLevel >= LOG_ERROR) level = ConsoleLogLevel::Error;

                ConsolePanel::AddLog(buffer, level);
            });

        NFD_Init(); 
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        EditorGUI::ApplyTheme();
        PropertyEditor::Init();

        // Register Panels
        m_Panels->Init();

        m_CommandHistory.SetNotifyCallback(
            []() { CH_CORE_TRACE("CommandHistory: Scene state changed, notifying editor..."); });

        // Auto-load last project/scene
        const auto &config = Editor::Get().GetEditorConfig();

        if (config.LoadLastProjectOnStartup && !config.LastProjectPath.empty() &&
            std::filesystem::exists(config.LastProjectPath))
        {
            CH_CORE_INFO("Auto-loading last project: {}", config.LastProjectPath);
            ProjectActions::Open(config.LastProjectPath);

            if (!config.LastScenePath.empty() && std::filesystem::exists(config.LastScenePath))
            {
                CH_CORE_INFO("Auto-loading last scene: {}", config.LastScenePath);
                SceneActions::Open(config.LastScenePath);
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

        CH_CORE_INFO("EditorLayer Attached with modular panels.");
        
        LoadEditorFonts();
    }

    void EditorLayer::LoadEditorFonts()
    {
        ImGuiIO &io = ImGui::GetIO();
        float fontSize = 16.0f;
        auto assetManager = Project::GetActive() ? Project::GetActive()->GetAssetManager() : nullptr;

        // Use a temporary asset manager if no project is active yet (for startup screen)
        std::unique_ptr<AssetManager> tempManager;
        if (!assetManager) {
            tempManager = std::make_unique<AssetManager>();
            tempManager->Initialize();
            assetManager = std::move(tempManager);
        }

        // --- Default UI Font (Lato) ---
        std::string fontPath = assetManager->ResolvePath("engine/resources/font/lato/lato-bold.ttf");
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
        std::string faPath = assetManager->ResolvePath("engine/resources/font/fa-solid-900.ttf");
        if (std::filesystem::exists(faPath))
        {
            static const ImWchar icons_ranges[] = {0xf000, 0xf8ff, 0}; 
            ImFontConfig icons_config;
            icons_config.MergeMode = true;
            icons_config.PixelSnapH = true;
            io.Fonts->AddFontFromFileTTF(faPath.c_str(), fontSize, &icons_config, icons_ranges);
            CH_CORE_INFO("Loaded and merged FontAwesome for editor: {}", faPath);
        }

        unsigned char *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    }

    void EditorLayer::OnDetach()
    {
        EditorContext::Shutdown();
        SetTraceLogCallback(nullptr);
        NFD_Quit();
    }

    void EditorLayer::OnUpdate(Timestep ts)
    {
        CH_PROFILE_FUNCTION();

        static int editorFrame = 0;
        if (editorFrame % 180 == 0) {
            //CH_CORE_INFO("[EDITOR_DIAG] OnUpdate - SceneState: {}", (int)EditorContext::GetSceneState());
        }
        editorFrame++;

        if (Input::IsKeyPressed(KEY_F11))
        {
            ToggleFullscreen();
        }

        if (auto scene = GetActiveScene())
        {
            if (EditorContext::GetSceneState() == SceneState::Play)
                scene->OnUpdateRuntime(ts);
            
            if (Input::IsKeyPressed(KEY_F5))
            {
                AppLaunchRuntimeEvent e;
                OnEvent(e);
            }

            m_Panels->OnUpdate(ts);
        }
    }

    void EditorLayer::OnRender(Timestep ts)
    {
        ClearBackground(BLACK);
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
                    viewportPanel->OnImGuiRender(true);
            }
            else
            {
                DrawDockSpace();
            }
        }
        else
        {
            if (auto projectBrowser = m_Panels->Get<ProjectBrowserPanel>())
                projectBrowser->OnImGuiRender();
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

        bool readOnly = EditorContext::GetSceneState() == SceneState::Play;
        m_Panels->OnImGuiRender(readOnly);

        DrawScriptUI();

        m_Layout->EndWorkspace();
    }

    void EditorLayer::DrawScriptUI()
    {
        if (EditorContext::GetSceneState() != SceneState::Play)
            return;

        auto scene = GetActiveScene();
        if (!scene)
            return;

        scene->GetRegistry().view<NativeScriptComponent>().each(
            [&](auto entity, auto &nsc)
            {
                for (auto &script : nsc.Scripts)
                {
                    if (script.Instance)
                        script.Instance->OnImGuiRender();
                }
            });
    }

    bool EditorLayer::OnProjectOpened(ProjectOpenedEvent &e)
    {
        CH_CORE_INFO("EditorLayer: Handling ProjectOpenedEvent - {}", e.GetPath());

        auto project = Project::GetActive();
        if (project)
        {
            if (auto contentBrowser = m_Panels->Get<ContentBrowserPanel>())
                contentBrowser->SetRootDirectory(Project::GetAssetDirectory());
        }
        return false;
    }

    bool EditorLayer::OnSceneOpened(SceneOpenedEvent &e)
    {
        auto activeScene = GetActiveScene();
        m_Panels->SetContext(activeScene);

        EditorContext::SetSelectedEntity({});

        // Sync project path
        auto project = Project::GetActive();
        if (project && !e.GetPath().empty())
        {
            project->SetActiveScenePath(std::filesystem::relative(e.GetPath(), project->GetProjectDirectory()));
            ProjectActions::Save();
        }
        return false;
    }

    void EditorLayer::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        
        // 1. Scene Management
        dispatcher.Dispatch<SceneOpenedEvent>(CH_BIND_EVENT_FN(EditorLayer::OnSceneOpened));
        dispatcher.Dispatch<ScenePlayEvent>([this](auto& e) { 
            CH_CORE_INFO("EditorLayer::OnEvent - ScenePlayEvent Received");
            SetSceneState(SceneState::Play); 
            return true; 
        });
        dispatcher.Dispatch<SceneStopEvent>([this](auto& e) { SetSceneState(SceneState::Edit); return true; });

        // 2. Project Management
        dispatcher.Dispatch<ProjectCreatedEvent>([](auto& ev) { ProjectActions::New(ev.GetProjectName(), ev.GetPath()); return true; });
        dispatcher.Dispatch<ProjectOpenedEvent>(CH_BIND_EVENT_FN(EditorLayer::OnProjectOpened));
        dispatcher.Dispatch<AppLaunchRuntimeEvent>([](auto& ev) { ProjectActions::LaunchStandalone(); return true; });

        // 3. Layout/System
        dispatcher.Dispatch<AppResetLayoutEvent>([this](auto& ev) { ResetLayout(); return true; });
        dispatcher.Dispatch<AppSaveLayoutEvent>([this](auto& ev) { m_Layout->SaveDefaultLayout(); return true; });
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
                auto newScene = std::make_shared<Scene>();
                RegisterGameScripts(newScene.get()); // Pre-register scripts for NativeScriptComponent
                SceneSerializer serializer(newScene.get());
                if (serializer.Deserialize(finalPath))
                {
                    if (m_RuntimeScene) m_RuntimeScene->OnRuntimeStop();
                    m_RuntimeScene = newScene;
                    m_RuntimeScene->OnRuntimeStart();
                    m_Panels->SetContext(m_RuntimeScene);
                    CH_CORE_INFO("Play Mode: Transitioned to scene {}", finalPath);
                }
                return true;
            }
            SceneActions::Open(finalPath); 
            return true; 
        });

        // 4. Selections/Picking
        dispatcher.Dispatch<EntitySelectedEvent>([this](auto& ev) {
            EditorContext::SetSelectedEntity(Entity(ev.GetEntity(), &ev.GetScene()->GetRegistry()));
            EditorContext::GetState().LastHitMeshIndex = ev.GetMeshIndex();
            return false;
        });

        // 5. Hierarchy propagation
        m_Panels->OnEvent(e);
        if (e.Handled) return;
        if (m_Actions->OnEvent(e)) return;

        // 6. Raw Input Overrides
        if (EditorContext::GetSceneState() == SceneState::Play)
        {
            if (auto activeScene = GetActiveScene())
                activeScene->OnEvent(e);
        }
        else if (e.GetEventType() == EventType::KeyPressed)
        {
            auto &ke = (KeyPressedEvent &)e;
            if (ke.GetKeyCode() == KEY_ESCAPE && EditorContext::GetState().FullscreenGame)
            {
                EditorContext::GetState().FullscreenGame = false;
                e.Handled = true;
            }
            if (auto activeScene = GetActiveScene())
                activeScene->OnEvent(e);
        }
    }

    CommandHistory &EditorLayer::GetCommandHistory()
    {
        return s_Instance->m_CommandHistory;
    }

    void EditorLayer::SetSceneState(SceneState state)
    {
        CH_CORE_INFO("EditorLayer::SetSceneState - Pending State: {}", (int)state);
        
        if (state == SceneState::Play)
        {
            if (EditorContext::GetSceneState() == SceneState::Play) return;

            CH_CORE_INFO("Editor: Play Mode Started");
            m_RuntimeScene = Scene::Copy(m_EditorScene);
            if (m_RuntimeScene)
            {
                EditorContext::SetSceneState(SceneState::Play);
                m_RuntimeScene->OnRuntimeStart();
                m_Panels->SetContext(m_RuntimeScene);
            }
            else
            {
                CH_CORE_ERROR("EditorLayer::SetSceneState - Failed to copy scene!");
            }
        }
        else
        {
            if (EditorContext::GetSceneState() == SceneState::Edit) return;

            CH_CORE_INFO("Editor: Play Mode Stopped");
            if (m_RuntimeScene)
            {
                m_RuntimeScene->OnRuntimeStop();
                m_RuntimeScene = nullptr;
            }

            EditorContext::SetSceneState(SceneState::Edit);
            m_Panels->SetContext(m_EditorScene);
        }
    }

    // Register game scripts (statically linked, defined in game_module.cpp outside any namespace)
    void EditorLayer::SetScene(std::shared_ptr<Scene> scene)
    {
        m_EditorScene = scene;
        EditorContext::SetSelectedEntity({});
        if (EditorContext::GetSceneState() == SceneState::Edit)
            m_Panels->SetContext(m_EditorScene);

        RegisterGameScripts(m_EditorScene.get());
    }
    void EditorLayer::SetViewportSize(const ImVec2& size)
    {
        m_ViewportSize = size;
        
        // Propagate to scenes to update camera aspect ratios
        if (m_EditorScene)
            m_EditorScene->OnViewportResize((uint32_t)size.x, (uint32_t)size.y);
            
        if (m_RuntimeScene)
            m_RuntimeScene->OnViewportResize((uint32_t)size.x, (uint32_t)size.y);
    }
} // namespace CHEngine
