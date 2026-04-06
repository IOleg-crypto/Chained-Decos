#include "editor_layer.h"
#include "editor_events.h"
#include "editor_gui.h"
#include "engine/core/input.h"
#include "engine/core/imgui_layer.h"

#include "engine/core/assets/asset_manager.h"
#include "engine/core/profiler.h"
#include "engine/physics/physics.h"

#include "engine/core/dialogs.h"
#include "engine/graphics/pipeline/render_command.h"
#include "engine/graphics/pipeline/ui_renderer.h"
#include "engine/scene/project.h"
#include "engine/scene/project_serializer.h"
#include "engine/scene/scene_serializer.h"

#include "imgui/IconsFontAwesome6.h"
#include "panels/console_panel.h"
#include "panels/content_browser_panel.h"
#include "panels/project_browser_panel.h"
#include "panels/property_editor.h"
#include "panels/viewport_panel.h"
#include "scripting/scene_scripting.h"
#include "scripting/script_file_system.h"
#include "scripting/scriptengine.h"
#include <ImGuizmo.h>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{
EditorLayer* EditorLayer::s_Instance = nullptr;

EditorLayer::EditorLayer()
    : Layer("EditorLayer")
{
    // Ensure the engine DLL uses the same ImGui context as the Editor
    ImGuiLayer::SetContext(ImGui::GetCurrentContext());

    s_Instance = this;
    EditorContext::Init();

    m_Layout = std::make_unique<EditorLayout>();
    m_Panels = std::make_unique<EditorPanels>();

    LoadConfig();
}

void EditorLayer::LoadConfig()
{
    std::filesystem::path configPath = ScriptFileSystem::GetExecutableDir() / "editor_settings.yaml";
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
                m_Config.LastProjectPath = node["LastProjectPath"].as<std::string>("");
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
    out << YAML::Key << "LastProjectPath" << YAML::Value << m_Config.LastProjectPath;
    out << YAML::Key << "LastScenePath" << YAML::Value << m_Config.LastScenePath;
    out << YAML::Key << "LoadLastProjectOnStartup" << YAML::Value << m_Config.LoadLastProjectOnStartup;
    out << YAML::Key << "AutoSaveEnabled" << YAML::Value << m_Config.AutoSaveEnabled;
    out << YAML::Key << "AutoSaveInterval" << YAML::Value << m_Config.AutoSaveInterval;
    out << YAML::EndMap;
    out << YAML::EndMap;

    std::filesystem::path configPath = ScriptFileSystem::GetExecutableDir() / "editor_settings.yaml";
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

    if (config.LoadLastProjectOnStartup && !config.LastProjectPath.empty() &&
        std::filesystem::exists(config.LastProjectPath))
    {
        CH_CORE_INFO("Auto-loading last project: {}", config.LastProjectPath);
        OpenProject(config.LastProjectPath);

        if (!config.LastScenePath.empty() && std::filesystem::exists(config.LastScenePath))
        {
            CH_CORE_INFO("Auto-loading last scene: {}", config.LastScenePath);
            OpenScene(config.LastScenePath);
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
    // SetTraceLogCallback(nullptr); // Removed: Raylib logging
}

void EditorLayer::OnUpdate(Timestep ts)
{
    CH_PROFILE_FUNCTION();

    // Update all panels (includes viewport camera controller)
    m_Panels->OnUpdate(ts);

    if (auto scene = GetActiveScene())
    {
        if (EditorContext::GetSceneState() == SceneState::Play)
        {
            if (ScriptEngine::Get().IsInitialized())
            {
                SceneScripting::Update(scene.get(), ts);
            }
            scene->OnUpdateRuntime(ts);

            // Handle deferred scene loading requested from C# scripts
            std::string pendingPath = ScriptEngine::Get().ConsumeRequestedScene();
            if (!pendingPath.empty())
            {
                SceneChangeRequestEvent ev(pendingPath);
                OnEvent(ev);
            }
        }
        else
        {
            scene->OnUpdateEditor(ts);

            // Auto-save logic
            if (m_Config.AutoSaveEnabled)
            {
                m_AutoSaveTimer += ts;
                if (m_AutoSaveTimer >= m_Config.AutoSaveInterval)
                {
                    AutoSaveScene();
                    m_AutoSaveTimer = 0.0f;
                }
            }
        }

        if (Input::IsKeyPressed(Key::F5))
        {
            AppLaunchRuntimeEvent e;
            OnEvent(e);
        }

        if (Input::IsKeyDown(Key::LeftControl) && Input::IsKeyPressed(Key::R))
        {
            ScriptEngine::Get().ReloadAssembly();
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

    m_Layout->EndWorkspace();
}

bool EditorLayer::OnProjectOpened(ProjectOpenedEvent& e)
{
    CH_CORE_INFO("EditorLayer: Handling ProjectOpenedEvent - {}", e.GetPath());

    auto project = Project::GetActive();
    if (project)
    {
        if (auto contentBrowser = m_Panels->Get<ContentBrowserPanel>())
        {
            contentBrowser->SetRootDirectory(Project::GetAssetDirectory());
        }
        ScriptEngine::Get().ReloadAssembly();

        m_Config.LastProjectPath = e.GetPath();
        SaveConfig();

        // Auto-load scene if available
        std::filesystem::path sceneToLoad;

        // 1. Try loading ActiveScene
        if (!project->GetConfig().ActiveScenePath.empty())
        {
            sceneToLoad = project->GetConfig().ProjectDirectory / project->GetConfig().ActiveScenePath;
        }

        // 2. Fallback to StartScene
        if (sceneToLoad.empty() || !std::filesystem::exists(sceneToLoad))
        {
            if (!project->GetConfig().StartScene.empty())
            {
                sceneToLoad = project->GetConfig().ProjectDirectory / project->GetConfig().AssetDirectory /
                              project->GetConfig().StartScene;
            }
        }

        // 3. Load the scene if found
        if (!sceneToLoad.empty() && std::filesystem::exists(sceneToLoad))
        {
            CH_CORE_INFO("EditorLayer: Auto-loading scene: {}", sceneToLoad.string());
            OpenScene(sceneToLoad);
        }
    }
    return false;
}

bool EditorLayer::OnSceneOpened(SceneOpenedEvent& e)
{
    auto activeScene = GetActiveScene();
    m_Panels->SetContext(activeScene);

    EditorContext::SetSelectedEntity({});

    // Sync project path
    auto project = Project::GetActive();
    if (project && !e.GetPath().empty())
    {
        project->SetActiveScenePath(std::filesystem::relative(e.GetPath(), project->GetProjectDirectory()));
        SaveProject();

        m_Config.LastScenePath = e.GetPath();
        SaveConfig();
    }

    // Sync Diagnostic Mode
    if (activeScene)
    {
        Renderer::Get().SetDiagnosticMode(activeScene->GetSettings().DiagnosticMode);
    }

    return false;
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
    dispatcher.Dispatch<SceneOpenedEvent>(CH_BIND_EVENT_FN(EditorLayer::OnSceneOpened));
    dispatcher.Dispatch<ScenePlayEvent>([this](auto& e) {
        CH_CORE_INFO("EditorLayer::OnEvent - ScenePlayEvent Received");
        SetSceneState(SceneState::Play);
        return true;
    });
    dispatcher.Dispatch<SceneStopEvent>([this](auto& e) {
        SetSceneState(SceneState::Edit);
        return true;
    });

    // 2. Project Management
    dispatcher.Dispatch<ProjectCreatedEvent>([this](auto& ev) {
        NewProject(ev.GetProjectName(), ev.GetPath());
        return true;
    });
    dispatcher.Dispatch<ProjectOpenedEvent>(CH_BIND_EVENT_FN(EditorLayer::OnProjectOpened));
    dispatcher.Dispatch<AppLaunchRuntimeEvent>([this](auto& ev) {
        LaunchStandalone();
        return true;
    });

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
            auto newScene = std::make_shared<Scene>();
            // RegisterGameScripts(newScene.get()); // Removed: now handled globally and copied to Scene
            SceneSerializer serializer(newScene.get());
            if (serializer.Deserialize(finalPath))
            {
                if (m_RuntimeScene)
                {
                    m_RuntimeScene->OnRuntimeStop();
                }
                m_RuntimeScene = newScene;
                m_RuntimeScene->OnRuntimeStart();
                CH_CORE_INFO("Play Mode: Transitioned to scene {}", finalPath);
            }
            return true;
        }
        OpenScene(finalPath);
        return true;
    });

    // 4. Selections/Picking
    dispatcher.Dispatch<EntitySelectedEvent>([this](auto& ev) {
        EditorContext::SetSelectedEntity(Entity(ev.GetEntity(), &ev.GetScene()->GetRegistry()));
        EditorContext::GetState().LastHitMeshIndex = ev.GetMeshIndex();
        return false;
    });

    if (e.Handled)
    {
        return;
    }
    bool handled = false;
    handled |= dispatcher.Dispatch<KeyPressedEvent>(CH_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
    handled |= dispatcher.Dispatch<MouseButtonPressedEvent>(CH_BIND_EVENT_FN(EditorLayer::OnMouseButtonPressed));
    if (handled)
    {
        return;
    }

    // 6. Raw Input Overrides
    if (EditorContext::GetSceneState() == SceneState::Play)
    {
        if (auto activeScene = GetActiveScene())
        {
            activeScene->OnEvent(e);
        }
    }
    else if (e.GetEventType() == EventType::KeyPressed)
    {
        auto& ke = (KeyPressedEvent&)e;
        if (ke.GetKeyCode() == Key::Escape && EditorContext::GetState().FullscreenGame)
        {
            EditorContext::GetState().FullscreenGame = false;
            e.Handled = true;
        }

        if (auto activeScene = GetActiveScene())
        {
            activeScene->OnEvent(e);
        }
    }
}

CommandHistory& EditorLayer::GetCommandHistory()
{
    return s_Instance->m_CommandHistory;
}

void EditorLayer::SetSceneState(SceneState state)
{
    CH_CORE_INFO("EditorLayer::SetSceneState - Pending State: {}", (int)state);

    if (state == SceneState::Play)
    {
        if (EditorContext::GetSceneState() == SceneState::Play)
        {
            return;
        }

        CH_CORE_INFO("Editor: Play Mode Started");
        m_RuntimeScene = Scene::Copy(m_EditorScene);
        if (m_RuntimeScene)
        {
            EditorContext::SetSceneState(SceneState::Play);
            m_RuntimeScene->OnRuntimeStart();
        }
        else
        {
            CH_CORE_ERROR("EditorLayer::SetSceneState - Failed to copy scene!");
        }
    }
    else
    {
        if (EditorContext::GetSceneState() == SceneState::Edit)
        {
            return;
        }

        CH_CORE_INFO("Editor: Play Mode Stopped");
        if (m_RuntimeScene)
        {
            SceneScripting::Stop(m_RuntimeScene.get());
            m_RuntimeScene->OnRuntimeStop();
            m_RuntimeScene = nullptr;
        }

        EditorContext::SetSceneState(SceneState::Edit);
    }
}

// Register game scripts (statically linked, defined in game_module.cpp outside any namespace)
void EditorLayer::SetScene(std::shared_ptr<Scene> scene)
{
    m_EditorScene = scene;
    EditorContext::SetSelectedEntity({});
    if (EditorContext::GetSceneState() == SceneState::Edit)
    {
    }

    // RegisterGameScripts(m_EditorScene.get()); // Removed: now handled globally
}
void EditorLayer::SetViewportSize(const ImVec2& size)
{
    m_ViewportSize = size;

    // Propagate to scenes to update camera aspect ratios
    if (m_EditorScene)
    {
        m_EditorScene->OnViewportResize((uint32_t)size.x, (uint32_t)size.y);
    }

    if (m_RuntimeScene)
    {
        m_RuntimeScene->OnViewportResize((uint32_t)size.x, (uint32_t)size.y);
    }
}

void EditorLayer::NewProject()
{
    // Simple default: close active project to show Project Browser
    Project::SetActive(nullptr);
}

void EditorLayer::NewProject(const std::string& name, const std::string& path)
{
    Project::New();
    auto project = Project::GetActive();
    project->GetConfig().Name = name;
    project->GetConfig().ProjectDirectory = path;

    ProjectSerializer serializer(project);
    serializer.Serialize((std::filesystem::path(path) / (name + ".chproject")).string());

    // Load engine shaders and resources for the dynamic newly created project
    Renderer::LoadEngineResources();
    UIRenderer::Get().LoadProjectFonts();
}

void EditorLayer::OpenProject()
{
    std::vector<FileDialogFilter> filters = {{"Chained Project", "chproject"}};
    auto result = Dialogs::OpenFile(filters);
    if (result)
    {
        OpenProject(*result);
    }
}

void EditorLayer::OpenProject(const std::filesystem::path& path)
{
    if (Project::Load(path))
    {
        EditorLayer::Get().SetLastProjectPath(path.string());

        // Load engine shaders and resources
        Renderer::LoadEngineResources();
        UIRenderer::Get().LoadProjectFonts();

        ProjectOpenedEvent e(path.string());
        Application::Get().OnEvent(e);
    }
}

void EditorLayer::SaveProject()
{
    auto project = Project::GetActive();
    ProjectSerializer serializer(project);
    serializer.Serialize((project->GetConfig().ProjectDirectory / (project->GetConfig().Name + ".chproject")).string());
}

static std::filesystem::path FindRuntimeExecutable(const std::string& projectName, const std::string& configStr)
{
    CH_PROFILE_FUNCTION();

    std::filesystem::path root;
#ifdef PROJECT_ROOT_DIR
    root = PROJECT_ROOT_DIR;
#else
    root = std::filesystem::current_path();
    while (root.has_parent_path() && !std::filesystem::exists(root / "CMakeLists.txt"))
    {
        root = root.parent_path();
    }
#endif

    if (!std::filesystem::exists(root))
    {
        CH_CORE_ERROR("FindRuntimeExecutable: Root path not found: {}", root.string());
        return {};
    }

    CH_CORE_INFO("Searching for runtime in: {}", root.string());

#ifdef CH_PLATFORM_WINDOWS
    std::string targetName = "ChainedRuntime.exe";
#else
    std::string targetName = "ChainedRuntime";
#endif

    // 1. Check directory of currently running editor (most reliable for portability)
    // We can't easily get the process handle here without platform code,
    // but we can check the current working directory bin folder
    std::filesystem::path currentBin = std::filesystem::current_path() / targetName;
    if (std::filesystem::exists(currentBin))
    {
        CH_CORE_INFO("FindRuntimeExecutable: Found in current directory: {}", currentBin.string());
        return currentBin;
    }

    // 2. Fast path: check common output locations including build presets
    std::vector<std::string> searchSubdirs = {"build/bin", "bin", "out/bin", "cmake-build-debug/bin",
                                              "cmake-build-release/bin"};

    // Add dynamic preset search build/*/bin
    if (std::filesystem::exists(root / "build"))
    {
        for (const auto& entry : std::filesystem::directory_iterator(root / "build"))
        {
            if (entry.is_directory())
            {
                std::filesystem::path p = entry.path() / "bin" / targetName;
                if (std::filesystem::exists(p))
                {
                    searchSubdirs.push_back("build/" + entry.path().filename().string() + "/bin");
                }
            }
        }
    }

    // Search collected paths
    for (const auto& sub : searchSubdirs)
    {
        std::filesystem::path p = root / sub / targetName;
        if (std::filesystem::exists(p))
        {
            CH_CORE_INFO("FindRuntimeExecutable: Path found at: {}", p.string());
            return p;
        }
    }

    // 3. Fallback: careful recursive search excluding noisy folders
    CH_CORE_INFO("FindRuntimeExecutable: Fast path failed, starting scoped recursive search...");
    try
    {
        for (auto it = std::filesystem::recursive_directory_iterator(root);
             it != std::filesystem::recursive_directory_iterator(); ++it)
        {
            const auto& entry = *it;
            auto filename = entry.path().filename().string();
            auto pathStr = entry.path().string();

            // Skip noisy/irrelevant directories
            if (entry.is_directory())
            {
                if (filename == ".git" || filename == ".cache" || filename == ".idea" || filename == "include" ||
                    filename == "engine")
                {
                    it.disable_recursion_pending();
                    continue;
                }
            }

            if (entry.is_regular_file() && filename == targetName)
            {
                CH_CORE_INFO("FindRuntimeExecutable: Deep search found at: {}", pathStr);
                return entry.path();
            }
        }
    } catch (const std::exception& e)
    {
        CH_CORE_WARN("FindRuntimeExecutable: Deep search error: {}", e.what());
    }

    CH_CORE_ERROR("FindRuntimeExecutable: Failed to find '{}' in {}", targetName, root.string());
    return {};
}

static std::string ResolveLaunchVariables(std::string str)
{
    CH_PROFILE_FUNCTION();

    auto project = Project::GetActive();
    if (!project)
    {
        return str;
    }

    std::filesystem::path root;
#ifdef PROJECT_ROOT_DIR
    root = PROJECT_ROOT_DIR;
#else
    root = std::filesystem::current_path();
    while (root.has_parent_path() && !std::filesystem::exists(root / "CMakeLists.txt"))
    {
        root = root.parent_path();
    }
#endif

    std::filesystem::path projectFile = project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
    std::string projectPathStr = std::filesystem::absolute(projectFile).string();

    // 1. Resolve ${ROOT}
    size_t pos = 0;
    while ((pos = str.find("${ROOT}")) != std::string::npos)
    {
        str.replace(pos, 7, std::filesystem::absolute(root).string());
    }

    // 2. Resolve ${PROJECT_FILE}
    while ((pos = str.find("${PROJECT_FILE}")) != std::string::npos)
    {
        str.replace(pos, 15, projectPathStr);
    }

    // 3. Resolve ${BUILD} - Intelligent discovery
    if (str.find("${BUILD}") != std::string::npos)
    {
        std::string configStr = (project->GetConfig().BuildConfig == Configuration::Release) ? "Release" : "Debug";
        std::filesystem::path exePath = FindRuntimeExecutable(project->GetConfig().Name, configStr);
        std::filesystem::path buildPath = exePath.parent_path();

        if (buildPath.empty())
        {
            // Last ditch effort if FindRuntimeExecutable failed
            std::vector<std::string> searchSubdirs = {"build/bin", "bin", "out/bin", "cmake-build-debug/bin",
                                                      "cmake-build-release/bin"};

            for (const auto& sub : searchSubdirs)
            {
                if (std::filesystem::exists(root / sub))
                {
                    buildPath = root / sub;
                    break;
                }
            }
        }

        while ((pos = str.find("${BUILD}")) != std::string::npos)
        {
            str.replace(pos, 8, std::filesystem::absolute(buildPath).string());
        }
    }

    return str;
}

void EditorLayer::LaunchStandalone()
{
    CH_PROFILE_FUNCTION();
    SaveProject();

    auto project = Project::GetActive();
    if (!project)
    {
        return;
    }

    auto& config = project->GetConfig();

    std::string runtimePath;
    std::string arguments;

    if (!config.LaunchProfiles.empty() && config.ActiveLaunchProfileIndex >= 0 &&
        config.ActiveLaunchProfileIndex < (int)config.LaunchProfiles.size())
    {
        const auto& profile = config.LaunchProfiles[config.ActiveLaunchProfileIndex];
        runtimePath = ResolveLaunchVariables(profile.BinaryPath);
        arguments = ResolveLaunchVariables(profile.Arguments);

        if (profile.UseDefaultArgs)
        {
            std::filesystem::path projectFile =
                project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
            arguments += std::format(" \"{}\"", std::filesystem::absolute(projectFile).string());
        }
    }
    else
    {
        // Fallback to old heuristic if no profiles
        CH_CORE_WARN("LaunchStandalone: No active launch profile. Falling back to heuristic search.");
        std::string configStr = (config.BuildConfig == Configuration::Release) ? "Release" : "Debug";
        runtimePath = FindRuntimeExecutable(config.Name, configStr).string();

        std::filesystem::path projectFile = project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
        arguments = std::format("\"{}\"", std::filesystem::absolute(projectFile).string());
    }

    if (runtimePath.empty() || !std::filesystem::exists(runtimePath))
    {
        CH_CORE_WARN("LaunchStandalone: Profile binary not found at '{}'. Searching heuristic...", runtimePath);
        std::string configStr = (config.BuildConfig == Configuration::Release) ? "Release" : "Debug";
        runtimePath = FindRuntimeExecutable(config.Name, configStr).string();

        if (runtimePath.empty())
        {
            CH_CORE_ERROR("LaunchStandalone: Runtime executable not found!");
            return;
        }
    }

#ifdef CH_PLATFORM_WINDOWS
    std::string command = std::format("start \"\" \"{}\" {}", runtimePath, arguments);
#else
    std::string command = std::format("\"{}\" {} &", runtimePath, arguments);
#endif
    CH_CORE_INFO("Launching Standalone: {}", command);

    system(command.c_str());
}

void EditorLayer::NewScene()
{
    auto newScene = std::make_shared<Scene>();

    // Ensure every scene starts with a Main Camera
    Entity camera = newScene->CreateEntity("Main Camera");
    auto& cc = camera.AddComponent<CameraComponent>();
    cc.Primary = true;
    camera.GetComponent<TransformComponent>().Translation = {0, 5, 10};

    EditorLayer::Get().SetScene(newScene);
}

void EditorLayer::OpenScene()
{
    std::vector<FileDialogFilter> filters = {{"Chained Scene", "chscene"}};
    auto result = Dialogs::OpenFile(filters);
    if (result)
    {
        OpenProject(*result);
    }
}

void EditorLayer::OpenScene(const std::filesystem::path& path)
{
    auto newScene = std::make_shared<Scene>();
    // KISS: Load Game Module BEFORE deserialization to ensure ScriptRegistry is populated
    EditorLayer::Get().SetScene(newScene);
    SceneSerializer serializer(newScene.get());

    if (serializer.Deserialize(path.string()))
    {
        // Sync environment if needed (optional, logic from Application::LoadScene can be moved here or to a helper)
        if (Project::GetActive() && Project::GetActive()->GetEnvironment())
        {
            if (newScene->GetSettings().Environment->GetPath().empty() &&
                newScene->GetSettings().Environment->GetSettings().Skybox.TexturePath.empty())
            {
                newScene->GetSettings().Environment = Project::GetActive()->GetEnvironment();
            }
        }

        // Sync with EditorLayer which manages the scene now
        // Assumes EditorLayer is active (SceneActions is editor-only code)
        newScene->GetSettings().ScenePath = path.string();

        SceneOpenedEvent e(path.string());
        EditorLayer::Get().SetLastScenePath(path.string());
        Application::Get().OnEvent(e);
    }
}

void EditorLayer::SaveScene()
{
    auto scene = EditorLayer::Get().GetActiveScene();
    if (scene->GetSettings().ScenePath.empty())
    {
        SaveSceneAs();
        return;
    }

    SceneSerializer serializer(scene.get());
    serializer.Serialize(scene->GetSettings().ScenePath);
    CH_INFO("Scene saved to {0}", scene->GetSettings().ScenePath);
}

void EditorLayer::SaveSceneAs()
{
    std::vector<FileDialogFilter> filters = {{"Chained Scene", "chscene"}};
    auto result = Dialogs::SaveFile(filters);
    if (result)
    {
        auto scene = EditorLayer::Get().GetActiveScene();
        scene->GetSettings().ScenePath = result->string();
        SceneSerializer serializer(scene.get());
        serializer.Serialize(result->string());
    }
}

void EditorLayer::AutoSaveScene()
{
    auto scene = EditorLayer::Get().GetActiveScene();
    if (!scene || scene->GetSettings().ScenePath.empty())
    {
        return;
    }

    SceneSerializer serializer(scene.get());
    serializer.Serialize(scene->GetSettings().ScenePath);
    CH_TRACE("Scene auto-saved to {0}", scene->GetSettings().ScenePath);
}

void EditorLayer::ReparentEntity(Entity child, Entity parent)
{
    if (child.HasComponent<HierarchyComponent>())
    {
        child.GetComponent<HierarchyComponent>().Parent = parent;
    }
}

bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
{
    if (e.IsRepeat())
    {
        return false;
    }

    bool ctrl = Input::IsKeyDown(Key::LeftControl) || Input::IsKeyDown(Key::RightControl);
    bool shift = Input::IsKeyDown(Key::LeftShift) || Input::IsKeyDown(Key::RightShift);

    auto keyCode = e.GetKeyCode();

    if (ctrl)
    {
        switch (keyCode)
        {
        case Key::N:
            NewScene();
            return true;
        case Key::O:
            OpenScene();
            return true;
        case Key::S:
            if (shift)
            {
                SaveSceneAs();
            }
            else
            {
                SaveScene();
            }
            return true;
        case Key::Z:
            m_CommandHistory.Undo();
            return true;
        case Key::Y:
            m_CommandHistory.Redo();
            return true;
        }
    }

    if (keyCode == Key::F5)
    {
        LaunchStandalone();
        return true;
    }

    return false;
}

bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
    return false;
}

} // namespace CHEngine
