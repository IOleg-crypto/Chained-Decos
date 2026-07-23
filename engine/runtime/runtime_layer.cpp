// runtime_layer.cpp
// Chained Engine — Runtime layer for standalone game execution.
// Handles project loading, scene transitions, scripting lifecycle, and the main runtime loop.

#include "runtime_layer.h"
#include "engine/app/application.h"
#include "engine/assets/asset_manager.h"
#include "engine/common/asset_path.h"
#include "engine/core/events/window_events.h"
#include "engine/core/platform.h"
#include "engine/core/service_locator.h"
#include "engine/core/window.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/ui/widget_renderer.h"
#include "engine/imgui/imgui_layer.h"
#include "engine/physics/physics.h"
#include "engine/project/project.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/scene_serializer.h"
#include "imgui.h"
#include "scripting/scene_scripting_manager.h"
#include "scripting/scriptengine.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <unordered_set>

namespace Chained
{

void AppendTextStyleFontRequest(const TextStyle& style, std::vector<std::pair<std::string, float>>& out,
                                std::unordered_set<std::string>& dedupe)
{
    std::string fontName = NormalizeAssetPath(style.FontName);
    if (fontName.empty() || fontName == "Default")
    {
        return;
    }

    const float fontSize = (style.FontSize > 0.0f) ? style.FontSize : 16.0f;
    const int roundedHalf = static_cast<int>(std::lround(fontSize * 2.0f));
    const std::string key = fontName + "|" + std::to_string(roundedHalf);

    if (!dedupe.insert(key).second)
    {
        return;
    }

    out.emplace_back(fontName, fontSize);
}

RuntimeLayer::RuntimeLayer(const std::string& projectPath)
    : Layer("RuntimeLayer"),
      m_ProjectPath(projectPath)
{
    m_SceneRenderer = std::make_unique<SceneRenderer>();

    // Safe here: Application's constructor already calls ServiceLocator::Lock() before
    // any layer is constructed (layers are pushed from CreateApplication after `new
    // Application(spec)` returns). Resolved once, reused for the layer's whole lifetime.
    m_Context.PhysicsSystem = ServiceLocator::Get<Physics>();
    m_Context.Scripting = ServiceLocator::Get<ScriptEngine>();
    m_Context.UI = ServiceLocator::TryGet<WidgetRenderer>(); // null in headless mode
    m_Renderer = ServiceLocator::Get<Renderer>();
    m_AssetManager = ServiceLocator::Get<AssetManager>();
}

RuntimeLayer::~RuntimeLayer()
{
}

void RuntimeLayer::OnAttach()
{
    auto* imguiLayer = Application::Get().GetImGuiLayer();
    if (imguiLayer)
    {
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(imguiLayer->GetContext()));
    }

    auto& io = ImGui::GetIO();

    // Add default font through DLL if needed, but usually redundant if Editor/Engine already did it
    if (io.Fonts->Fonts.Size == 0)
    {
        io.Fonts->AddFontDefault();
        CH_CORE_INFO("RuntimeSystem: Using built-in ImGui default font.");
    }

    InitProject(m_ProjectPath);

    if (ImFont* projectDefaultFont =
            ServiceLocator::Get<WidgetRenderer>()->GetFontRegistry().EnsureDefaultProjectFont(18.0f, false))
    {
        io.FontDefault = projectDefaultFont;
        CH_CORE_INFO("RuntimeSystem: Switched default UI font to project font.");
    }

    if (imguiLayer)
    {
        imguiLayer->RefreshFontAtlasTexture();
    }

    // Ensure camera aspect ratio is correct on startup
    if (m_Scene)
    {
        Window& window = Application::Get().GetWindow();
        m_Scene->OnViewportResize(window.GetWidth(), window.GetHeight());
        EnsureRuntimeFramebuffer((uint32_t)window.GetWidth(), (uint32_t)window.GetHeight());
    }
}

void RuntimeLayer::OnDetach()
{
    StopCurrentScene();

    m_Scene = nullptr;
    ServiceLocator::Get<ScriptEngine>()->SetContextScene(nullptr);
    m_RuntimeStarted = false;
    m_IsSceneLoading = false;
    m_LoadingOverlayElapsed = 0.0f;
}

void RuntimeLayer::OnUpdate(Timestep ts)
{
    if (!m_PendingScenePath.empty())
    {
        std::string path = std::move(m_PendingScenePath);
        m_PendingScenePath.clear();
        LoadScene(path);
        return;
    }

    if (auto* uiRenderer = ServiceLocator::TryGet<WidgetRenderer>())
    {
        if (uiRenderer->GetFontRegistry().NeedsAtlasRebuild())
        {
            auto* imguiLayer = Application::Get().GetImGuiLayer();
            if (imguiLayer)
            {
                imguiLayer->ExecuteNextFrame([imguiLayer]() { imguiLayer->RefreshFontAtlasTexture(); });
            }
            uiRenderer->GetFontRegistry().ClearRebuildFlag();
        }
    }

    if (m_Scene && m_IsSceneLoading)
    {
        m_LoadingOverlayElapsed += (float)ts;

        if (IsSceneReadyToStart() && m_LoadingOverlayElapsed >= m_LoadingOverlayMinDuration)
        {
            // Clear stale button press flags from the previous scene before starting runtime.
            // Without this, a button press that triggered the scene change would still be "pressed"
            // on the first frame of the new scene, causing immediate unintended transitions.
            ServiceLocator::Get<WidgetRenderer>()->ResetButtonStates(m_Scene.get());
            // TransitionToState handles OnRuntimeStart internally — this is the single call site.
            m_Scene->TransitionToState(SceneState::Play, m_Context);
            m_RuntimeStarted = true;
            m_IsSceneLoading = false;
            m_SuppressNextUIInput = true;
            CH_CORE_INFO("RuntimeSystem: Scene assets are ready, entering runtime.");
        }
    }

    if (m_Scene && m_RuntimeStarted)
    {
        // Process UI input BEFORE scripts run, unconditionally each frame. This
        // guarantees PressedThisFrame is reset every frame regardless of whether
        // the canvas is drawn this frame (BeginChild can be skipped when the
        // window is collapsed/clipped), so a one-frame click edge never sticks
        // and re-fires script actions on subsequent frames.
        bool suppress = m_SuppressNextUIInput;
        m_SuppressNextUIInput = false;
        ServiceLocator::Get<WidgetRenderer>()->ProcessInput(m_Scene.get(), suppress);

        m_Scene->OnUpdateRuntime(ts, m_Context);
    }

    if (m_IsBoostingUploads)
    {
        m_BoostUploadsTimer -= ts;
        if (m_BoostUploadsTimer <= 0.0f)
        {
            m_IsBoostingUploads = false;
        }
    }
}

void RuntimeLayer::OnRender(Timestep ts)
{
    Window& window = Application::Get().GetWindow();
    uint32_t width = (uint32_t)window.GetWidth();
    uint32_t height = (uint32_t)window.GetHeight();

    if (!m_Scene)
    {
        m_Renderer->Clear({0.0f, 0.0f, 0.0f, 1.0f});
        return;
    }

    if (width == 0 || height == 0)
    {
        return;
    }

    EnsureRuntimeFramebuffer(width, height);

    const auto& settings = m_Scene->GetSettings();
    glm::vec4 bgColor = {settings.BackgroundColor.r / 255.0f, settings.BackgroundColor.g / 255.0f,
                         settings.BackgroundColor.b / 255.0f, settings.BackgroundColor.a / 255.0f};

    if (settings.Environment && settings.Mode != BackgroundMode::Color)
    {
        auto& env = settings.Environment->GetSettings();
        if (env.Fog.Enabled)
        {
            bgColor = glm::vec4(env.Fog.FogColor.r / 255.0f, env.Fog.FogColor.g / 255.0f, env.Fog.FogColor.b / 255.0f,
                                env.Fog.FogColor.a / 255.0f);
        }
    }

    auto camera = GetActiveCamera();
    if (camera)
    {
        float nearClip = 0.01f;
        float farClip = 10000.0f;

        Entity primaryCam = SceneRenderer::GetPrimaryCameraEntity(m_Scene->GetRegistry(), m_Scene->GetRegistryPtr());
        if (primaryCam && primaryCam.HasComponent<CameraComponent>())
        {
            auto& cameraComp = primaryCam.GetComponent<CameraComponent>().Camera;
            nearClip = cameraComp.GetPerspectiveNearClip();
            farClip = cameraComp.GetPerspectiveFarClip();
        }

        SceneRenderOptions options;

        m_HDRFramebuffer->Bind();
        m_Renderer->Clear(bgColor);
        m_SceneRenderer->RenderScene(m_Scene->GetRegistry(), m_Scene->GetSettings(), camera.value(), nearClip, farClip,
                                     options);
        m_HDRFramebuffer->Unbind();
        m_HDRFramebuffer->Resolve();

        m_Renderer->SetViewport(0, 0, (int)width, (int)height);
        m_Renderer->Clear(bgColor);
        m_Renderer->ApplyPostProcessing(m_HDRFramebuffer->GetColorAttachmentRendererID(),
                                        m_HDRFramebuffer->GetDepthAttachmentRendererID(), camera.value(), nullptr, {});

        ShaderAsset* overrideShader = nullptr;
        std::vector<ShaderUniform> uniforms;

        if (primaryCam && primaryCam.HasComponent<ShaderComponent>())
        {
            auto& sc = primaryCam.GetComponent<ShaderComponent>();
            if (sc.Enabled && !sc.ShaderPath.empty())
            {
                auto asset = m_AssetManager->Get<ShaderAsset>(sc.ShaderPath);
                if (asset)
                {
                    overrideShader = asset.get();
                    uniforms = sc.Uniforms;
                }
            }
        }
    }
    else
    {
        m_Renderer->Clear(bgColor);
    }
}

void RuntimeLayer::OnImGuiRender()
{
    if (m_Scene)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking;

        // Allow inputs for runtime UI
        flags &= ~ImGuiWindowFlags_NoInputs;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        if (ImGui::Begin("RuntimeUI", nullptr, flags))
        {
            if (m_RuntimeStarted)
            {
                ImVec2 childSize = ImGui::GetContentRegionAvail();
                if (ImGui::BeginChild("##RuntimeUICanvas", childSize, false,
                                      ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                                          ImGuiWindowFlags_NoScrollWithMouse))
                {
                    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
                    ServiceLocator::Get<WidgetRenderer>()->DrawCanvas(m_Scene.get(), canvasPos, canvasSize, false);
                    m_Scene->OnRenderUI();
                }
                ImGui::EndChild();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);

        if (m_IsSceneLoading)
        {
            DrawLoadingOverlay();
        }
    }
}

void RuntimeLayer::OnEvent(Event& e)
{

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](auto& ev) {
        if (ev.GetWidth() == 0 || ev.GetHeight() == 0)
        {
            return false;
        }

        EnsureRuntimeFramebuffer(ev.GetWidth(), ev.GetHeight());
        if (m_Scene)
        {
            m_Scene->OnViewportResize(ev.GetWidth(), ev.GetHeight());
        }
        return false;
    });

    dispatcher.Dispatch<SceneChangeRequestEvent>([this](auto& ev) {
        m_PendingScenePath = ev.GetPath();
        return true;
    });
}

//-----------------------------------------------------------------------------
// Purpose: Load a new scene from file
//-----------------------------------------------------------------------------
void RuntimeLayer::LoadScene(const std::string& path)
{
    const std::string normalizedPath = NormalizeScenePath(path);
    if (normalizedPath.empty())
    {
        CH_CORE_WARN("RuntimeSystem: Ignoring empty scene path request.");
        return;
    }

    std::filesystem::path scenePath = normalizedPath;
    if (scenePath.is_relative() && Project::GetActive())
    {
        scenePath = Project::GetAssetPath(scenePath);
    }

    if (!scenePath.is_absolute())
    {
        std::error_code ec;
        scenePath = std::filesystem::absolute(scenePath, ec);
        if (ec)
        {
            CH_CORE_ERROR("RuntimeSystem: Failed to resolve absolute scene path '{}' ({})", normalizedPath,
                          ec.message());
            return;
        }
    }

    if (!FileExists(scenePath))
    {
        CH_CORE_ERROR("RuntimeSystem: Scene file not found '{}'.", scenePath.string());
        return;
    }

    if (!TransitionToScene(scenePath))
    {
        CH_CORE_ERROR("RuntimeSystem: Failed to transition to scene '{}'.", scenePath.string());
    }
}

void RuntimeLayer::LoadScene(int index)
{
    auto project = Project::GetActive();
    if (!project)
    {
        return;
    }

    const auto& buildScenes = project->GetConfig().BuildScenes;
    if (index >= 0 && index < (int)buildScenes.size())
    {
        std::filesystem::path fullPath = Project::GetAssetPath(buildScenes[index]);
        LoadScene(fullPath.string());
    }
}

bool RuntimeLayer::InitProject(const std::string& projectPath)
{
    if (!DiscoverAndLoadProject(projectPath))
    {
        return false;
    }

    auto project = Project::GetActive();
    if (!project)
    {
        CH_CORE_ERROR("Runtime: No active project after DiscoverAndLoadProject - cannot initialize scripting.");
        return false;
    }
    auto assemblyPath =
        ScriptEngine::ResolveAssemblyPath(project->GetConfig().Scripting, project->GetConfig().ProjectDirectory);

    CH_CORE_INFO("RuntimeSystem: Loading project assembly: {}", assemblyPath.string());

    // Initialize Scripting for the loaded project
    if (assemblyPath.empty() || !ServiceLocator::Get<ScriptEngine>()->ReloadAssembly(assemblyPath.string()))
    {
        CH_CORE_WARN("RuntimeSystem: Script reload failed during project initialization (path: {}). Runtime continues "
                     "without scripts.",
                     assemblyPath.string());
    }

    // Discover project fonts once before any scene loads.
    ServiceLocator::Get<WidgetRenderer>()->LoadProjectFonts();

    ApplyWindowConfiguration();
    SetupBrandingAndIcon();

    LoadInitialScene();

    return true;
}

bool RuntimeLayer::DiscoverAndLoadProject(const std::string& projectPath)
{
    std::filesystem::path discoveryPath = projectPath;

    if (discoveryPath.empty())
    {
        std::filesystem::path exePath = std::filesystem::absolute(
            std::filesystem::path(Application::Get().GetSpecification().CommandLineArgs.Args[0]));
        discoveryPath = exePath.parent_path();
    }

    if (discoveryPath.extension() == ".chproject")
    {
        m_ProjectPath = discoveryPath.string();
    }
    else
    {
        m_ProjectPath = (discoveryPath / (Application::Get().GetSpecification().Name + ".chproject")).string();
    }

    if (m_ProjectPath.empty())
    {
        return false;
    }

    auto project = Project::Load(m_ProjectPath);
    if (!project)
    {
        CH_CORE_ERROR("RuntimeSystem: Failed to load project file at '{}'", m_ProjectPath);
        return false;
    }

    CH_CORE_INFO("RuntimeSystem: Project loaded: {}", project->GetConfig().Name);
    CH_CORE_INFO("RuntimeSystem: Project Directory: {}", project->GetProjectDirectoryForProject().string());
    CH_CORE_INFO("RuntimeSystem: Asset Directory: {}", Project::GetAssetDirectory().string());

    m_AssetManager->SetProjectDirectory(project->GetProjectDirectoryForProject());
    m_AssetManager->SetAssetDirectory(Project::GetAssetDirectory());

    // Open resources.pack if it exists next to the executable
    std::filesystem::path packPath = Platform::GetExecutableDirectory() / "resources.pack";
    if (std::filesystem::exists(packPath))
    {
        m_AssetManager->OpenPack(packPath);
    }

    // CRITICAL: Load engine shaders and resources immediately after project is resolved
    m_Renderer->LoadEngineResources();

    ServiceLocator::Get<ScriptEngine>()->TryAutoLoad(project->GetConfig());

    return true;
}

void RuntimeLayer::ApplyWindowConfiguration()
{
    auto project = Project::GetActive();
    if (!project)
    {
        return;
    }

    auto& config = project->GetConfig();
    Window& window = Application::Get().GetWindow();

    bool vsync = config.Window.VSync;
    int width = config.Window.Width;
    int height = config.Window.Height;
    bool fullscreen = config.Runtime.Fullscreen;

    const auto& args = Application::Get().GetSpecification().CommandLineArgs;
    for (int i = 1; i < args.Count; ++i)
    {
        std::string arg = args.Args[i];
        if (arg == "--width" && i + 1 < args.Count)
        {
            width = std::stoi(args.Args[++i]);
        }
        else if (arg == "--height" && i + 1 < args.Count)
        {
            height = std::stoi(args.Args[++i]);
        }
        else if (arg == "--fullscreen")
        {
            fullscreen = true;
        }
        else if (arg == "--windowed")
        {
            fullscreen = false;
        }
        else if (arg == "--vsync" && i + 1 < args.Count)
        {
            vsync = (std::string(args.Args[++i]) == "on");
        }
    }

    window.SetVSync(vsync);
    if (width != window.GetWidth() || height != window.GetHeight())
    {
        window.SetSize(width, height);
    }
    window.SetFullscreen(fullscreen);
}

void RuntimeLayer::SetupBrandingAndIcon()
{
    auto project = Project::GetActive();
    if (!project)
    {
        return;
    }

    auto& config = project->GetConfig();
    Window& window = Application::Get().GetWindow();
    window.SetTitle(config.Name);

    if (config.IconPath.empty())
    {
        return;
    }

    std::filesystem::path iconPath = "";
    std::string resolved = config.IconPath;
    if (!resolved.empty() && std::filesystem::exists(resolved))
    {
        iconPath = resolved;
    }
    else if (std::filesystem::exists(config.IconPath))
    {
        iconPath = config.IconPath;
    }

    if (!iconPath.empty())
    {
        CH_CORE_INFO("RuntimeSystem: Setting window icon: {}", iconPath.string());
        window.SetWindowIcon(iconPath.string());
    }
    else
    {
        CH_CORE_WARN("RuntimeSystem: Failed to resolve window icon: {}", config.IconPath);
    }
}

void RuntimeLayer::LoadInitialScene()
{
    auto project = Project::GetActive();
    if (!project)
    {
        return;
    }

    auto& config = project->GetConfig();
    std::string sceneToLoad = config.StartScene;

    const auto& args = Application::Get().GetSpecification().CommandLineArgs;
    for (int i = 1; i < args.Count; ++i)
    {
        if (std::string(args.Args[i]) == "--scene" && i + 1 < args.Count)
        {
            sceneToLoad = args.Args[++i];
            break;
        }
    }

    if (sceneToLoad.empty())
    {
        sceneToLoad = config.ActiveScenePath.string();
    }

    CH_CORE_INFO("RuntimeSystem: Initial scene to load: '{}'", sceneToLoad);

    if (sceneToLoad.empty())
    {
        std::filesystem::path scenesDir = Project::GetAssetDirectory() / "scenes";
        if (std::filesystem::exists(scenesDir))
        {
            try
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(scenesDir))
                {
                    if (entry.path().extension() == ".chscene")
                    {
                        sceneToLoad = std::filesystem::relative(entry.path(), Project::GetAssetDirectory()).string();
                        break;
                    }
                }
            } catch (const std::filesystem::filesystem_error& e)
            {
                CH_CORE_WARN("RuntimeSystem: Failed to enumerate scenes in '{}': {}", scenesDir.string(), e.what());
            }
        }
    }

    if (!sceneToLoad.empty())
    {
        LoadScene(sceneToLoad);
    }
}

std::string RuntimeLayer::NormalizeScenePath(const std::string& path) const
{
    return NormalizeAssetPath(path);
}

void RuntimeLayer::StopCurrentScene()
{
    if (!m_Scene)
    {
        return;
    }

    if (m_RuntimeStarted)
    {
        m_Scene->OnRuntimeStop(m_Context);
    }
}

std::vector<std::pair<std::string, float>> RuntimeLayer::CollectSceneFontRequests() const
{
    std::vector<std::pair<std::string, float>> requests;
    if (!m_Scene)
    {
        return requests;
    }

    std::unordered_set<std::string> dedupe;
    auto& registry = m_Scene->GetRegistry();

    auto view = registry.view<UIControlComponent>();
    for (entt::entity id : view)
    {
        AppendTextStyleFontRequest(view.get<UIControlComponent>(id).TextStyle, requests, dedupe);
    }

    return requests;
}

void RuntimeLayer::PreloadSceneFonts(bool allowRuntimeMutation)
{
    auto requests = CollectSceneFontRequests();
    if (requests.empty())
    {
        return;
    }

    const int loadedCount =
        ServiceLocator::Get<WidgetRenderer>()->GetFontRegistry().PreloadFonts(requests, allowRuntimeMutation);
    if (loadedCount <= 0)
    {
        return;
    }

    CH_CORE_INFO("RuntimeSystem: Preloaded {} scene font tuple(s).", loadedCount);

    if (allowRuntimeMutation && ImGui::GetFrameCount() > 0)
    {
        if (auto* imguiLayer = Application::Get().GetImGuiLayer())
        {
            if (!imguiLayer->RefreshFontAtlasTexture())
            {
                CH_CORE_WARN("RuntimeSystem: Scene fonts were loaded, but font atlas refresh failed.");
            }
        }
    }
}

bool RuntimeLayer::TransitionToScene(const std::filesystem::path& scenePath)
{
    StopCurrentScene();

    m_RuntimeStarted = false;
    m_IsSceneLoading = false;
    m_LoadingOverlayElapsed = 0.0f;

    // Runtime scenes need the active ScriptEngine so ManagedScriptComponent instances can be created.

    auto nextScene = std::make_shared<Scene>();
    SceneSerializer serializer(nextScene.get());
    if (!serializer.Deserialize(scenePath.string()))
    {
        m_Scene = nullptr;
        ServiceLocator::Get<ScriptEngine>()->SetContextScene(nullptr);
        return false;
    }

    m_Scene = nextScene;
    m_Scene->GetSettings().ScenePath = scenePath.string();

    // Bind the event callback so SceneTransitionComponents can dispatch SceneChangeRequestEvent
    // back to RuntimeLayer::OnEvent, which handles the actual scene loading.
    m_Scene->SetEventCallback([this](Event& e) { OnEvent(e); });

    // Keep current ScriptEngine behavior intact while runtime owns transition flow.
    ServiceLocator::Get<ScriptEngine>()->SetContextScene(m_Scene.get());

    // NOTE: Scene is left in Edit state here. TransitionToState(Play) — which calls
    // OnRuntimeStart — will happen once in OnUpdate when all async assets are ready.
    // This prevents a double OnRuntimeStart that caused a segfault.

    Window& window = Application::Get().GetWindow();
    m_Scene->OnViewportResize(window.GetWidth(), window.GetHeight());
    EnsureRuntimeFramebuffer((uint32_t)window.GetWidth(), (uint32_t)window.GetHeight());

    // Ensure no stale button press flags survive from before the scene transition.
    // Scripts run in OnUpdate (before ImGui DrawCanvas which normally resets these),
    // so without this, ExitScript or SceneScript would fire on the very first frame.
    {
        auto& registry = m_Scene->GetRegistry();
        auto view = registry.view<UIControlComponent>();
        for (entt::entity id : view)
        {
            view.get<UIControlComponent>(id).PressedThisFrame = false;
        }
    }

    // Suppress UI input for the entire loading phase so that a stale ImGui
    // IsMouseClicked edge (from the click that triggered the scene change) cannot
    // fire button callbacks during the loading-overlay frames before Play starts.
    m_SuppressNextUIInput = true;

    PreloadSceneFonts(ImGui::GetFrameCount() > 0);

    m_IsBoostingUploads = true;
    m_BoostUploadsTimer = 5.0f;
    CH_CORE_INFO("RuntimeSystem: Boosting asset uploads for scene loading...");

    m_IsSceneLoading = true;
    m_LoadingOverlayElapsed = 0.0f;
    CH_CORE_INFO("RuntimeSystem: Scene loaded, waiting for async assets before runtime start.");
    return true;
}

std::optional<Camera3D> RuntimeLayer::GetActiveCamera()
{
    if (m_Scene)
    {
        return SceneRenderer::GetActiveCamera(m_Scene->GetRegistry());
    }
    return std::nullopt;
}

void RuntimeLayer::EnsureRuntimeFramebuffer(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    auto project = Project::GetActive();
    int samples = project ? project->GetConfig().Render.AntiAliasingSamples : 4;
    uint32_t configuredSamples = samples > 1 ? (uint32_t)samples : 1u;

    if (m_HDRFramebuffer && configuredSamples != m_HDRFramebufferSamples)
    {
        m_HDRFramebuffer.reset();
    }

    if (!m_HDRFramebuffer)
    {
        FramebufferSpecification hdrSpec;
        hdrSpec.Width = width;
        hdrSpec.Height = height;
        hdrSpec.Samples = configuredSamples;
        hdrSpec.ColorFormat = FramebufferColorFormat::RGBA16F;
        m_HDRFramebuffer = Framebuffer::Create(hdrSpec);
        m_HDRFramebufferSamples = configuredSamples;
    }
    else
    {
        const auto& spec = m_HDRFramebuffer->GetSpecification();
        if (spec.Width != width || spec.Height != height)
        {
            m_HDRFramebuffer->Resize(width, height);
        }
    }
}

bool RuntimeLayer::IsSceneReadyToStart() const
{
    auto* assetManager = m_AssetManager;
    if (!assetManager)
    {
        return true;
    }
    return !assetManager->HasBackgroundWork();
}

void RuntimeLayer::DrawLoadingOverlay()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.02f, 0.02f, 0.92f));

    if (ImGui::Begin("##RuntimeLoadingOverlay", nullptr, flags))
    {
        const size_t totalPending = m_AssetManager->GetLoadingAssetCount();

        int dotsCount = (static_cast<int>(ImGui::GetTime() * 2.5f) % 3) + 1;
        std::string dots(static_cast<size_t>(dotsCount), '.');
        std::string loadingLine = "Preparing world" + dots;
        std::string pendingLine = "Pending assets: " + std::to_string(totalPending);

        ImGui::SetCursorPosY(ImGui::GetWindowHeight() * 0.45f);

        const char* title = "Loading scene";
        ImVec2 titleSize = ImGui::CalcTextSize(title);
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
        ImGui::TextUnformatted(title);

        ImVec2 loadingSize = ImGui::CalcTextSize(loadingLine.c_str());
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - loadingSize.x) * 0.5f);
        ImGui::TextUnformatted(loadingLine.c_str());

        ImVec2 pendingSize = ImGui::CalcTextSize(pendingLine.c_str());
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - pendingSize.x) * 0.5f);
        ImGui::TextUnformatted(pendingLine.c_str());
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}
} // namespace Chained
