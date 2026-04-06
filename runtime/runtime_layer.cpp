//========= Copyright Chained Decos, All rights reserved. ============//
//
// Purpose: Primary gameplay runtime layer.
//          Handles scene management, game loop, and script execution.
//
//=============================================================================//

#include "runtime_layer.h"
#include "engine/core/application.h"
#include "engine/core/window.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/scene_renderer.h"
#include "engine/graphics/pipeline/ui_renderer.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/scene_serializer.h"
#include "imgui.h"
#include "scripting/scene_scripting.h"
#include "scripting/scriptengine.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>


namespace CHEngine
{
RuntimeLayer::RuntimeLayer(const std::string& projectPath)
    : Layer("RuntimeLayer"),
      m_ProjectPath(projectPath)
{
    m_SceneRenderer = std::make_unique<SceneRenderer>();
}

RuntimeLayer::~RuntimeLayer()
{
}

void RuntimeLayer::OnAttach()
{
    if (InitProject(m_ProjectPath))
    {
        // Initial scene/module load is handled by InitProject calling LoadInitialScene
    }

    ImGuiIO& io = ImGui::GetIO();
    io.FontDefault = io.Fonts->AddFontDefault();
    CH_CORE_INFO("RuntimeLayer: Using built-in ImGui default font.");

    // Register project-specific fonts into ImGui's atlas.
    // Must happen AFTER all AddFontFromFileTTF() calls and BEFORE the atlas is built by rlImGui.
    UIRenderer::Get().LoadProjectFonts();

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
    if (m_Scene)
    {
        if (m_RuntimeStarted)
        {
            SceneScripting::OnRuntimeStop(m_Scene.get());
            m_Scene->OnRuntimeStop();
        }
        SceneScripting::Stop(m_Scene.get());
    }

    m_RuntimeStarted = false;
    m_IsSceneLoading = false;
    m_LoadingOverlayElapsed = 0.0f;
}

void RuntimeLayer::OnUpdate(Timestep ts)
{
    std::string pendingPath = ScriptEngine::Get().ConsumeRequestedScene();
    if (!pendingPath.empty())
    {
        LoadScene(pendingPath);
    }

    if (m_Scene && m_IsSceneLoading)
    {
        m_LoadingOverlayElapsed += (float)ts;

        if (IsSceneReadyToStart() && m_LoadingOverlayElapsed >= m_LoadingOverlayMinDuration)
        {
            SceneScripting::OnRuntimeStart(m_Scene.get());
            m_Scene->OnRuntimeStart();
            m_RuntimeStarted = true;
            m_IsSceneLoading = false;
            CH_CORE_INFO("RuntimeLayer: Scene assets are ready, entering runtime.");
        }
    }

    if (m_Scene && m_RuntimeStarted)
    {
        SceneScripting::Update(m_Scene.get(), ts);
        m_Scene->OnUpdateRuntime(ts);
    }

    if (m_IsBoostingUploads)
    {
        m_BoostUploadsTimer -= ts;
        if (m_BoostUploadsTimer <= 0.0f)
        {
            m_IsBoostingUploads = false;
            // Limit reset removed as per user request
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
        Renderer::Get().Clear({0.0f, 0.0f, 0.0f, 1.0f});
        return;
    }

    if (width == 0 || height == 0)
    {
        return;
    }

    EnsureRuntimeFramebuffer(width, height);

    glm::vec4 bgColor = {0.0f, 0.0f, 0.0f, 1.0f};

    if (m_Scene->GetSettings().Environment)
    {
        auto& env = m_Scene->GetSettings().Environment->GetSettings();
        if (env.Fog.Enabled)
        {
            bgColor = glm::vec4(env.Fog.FogColor.r / 255.0f, env.Fog.FogColor.g / 255.0f, 
                               env.Fog.FogColor.b / 255.0f, env.Fog.FogColor.a / 255.0f);
        }
    }

    auto camera = GetActiveCamera();
    if (camera)
    {
        float nearClip = 0.01f;
        float farClip = 1000.0f;

        Entity primaryCam = m_Scene->GetPrimaryCameraEntity();
        if (primaryCam && primaryCam.HasComponent<CameraComponent>())
        {
            auto& cameraComp = primaryCam.GetComponent<CameraComponent>().Camera;
            nearClip = cameraComp.GetPerspectiveNearClip();
            farClip = cameraComp.GetPerspectiveFarClip();
        }

        SceneRenderOptions options;
        if (auto project = Project::GetActive())
        {
            options.TargetFPS = project->GetConfig().Animation.TargetFPS;
        }
        options.ShowEditorIcons = false;

        m_HDRFramebuffer->Bind();
        Renderer::Get().Clear(bgColor);
        m_SceneRenderer->RenderScene(m_Scene.get(), camera.value(), nearClip, farClip, ts, options);
        m_HDRFramebuffer->Unbind();

        Renderer::Get().SetViewport(0, 0, (int)width, (int)height);
        Renderer::Get().ApplyPostProcessing(
            m_HDRFramebuffer->GetColorAttachmentRendererID(),
            m_HDRFramebuffer->GetDepthAttachmentRendererID(),
            camera.value());
    }
    else
    {
        Renderer::Get().Clear(bgColor);
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
                    UIRenderer::Get().DrawCanvas(m_Scene.get(), canvasPos, canvasSize, false);
                    SceneScripting::RenderUI(m_Scene.get());
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
    if (m_Scene && m_RuntimeStarted)
    {
        SceneScripting::DispatchEvent(m_Scene.get(), e);
        m_Scene->OnEvent(e);
    }

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
        ScriptEngine::Get().RequestLoadScene(ev.GetPath());
        return true;
    });
}

//-----------------------------------------------------------------------------
// Purpose: Load a new scene from file
//-----------------------------------------------------------------------------
void RuntimeLayer::LoadScene(const std::string& path)
{
    std::string normalizedPath = path;
    normalizedPath.erase(
        normalizedPath.begin(),
        std::find_if(normalizedPath.begin(), normalizedPath.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    normalizedPath.erase(
        std::find_if(normalizedPath.rbegin(), normalizedPath.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(),
        normalizedPath.end());

    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    if (normalizedPath.rfind("assets/", 0) == 0)
    {
        normalizedPath = normalizedPath.substr(7);
    }

    if (normalizedPath.empty())
    {
        CH_CORE_WARN("RuntimeLayer: Ignoring empty scene path request.");
        return;
    }

    std::filesystem::path scenePath = normalizedPath;
    if (scenePath.is_relative() && Project::GetActive())
    {
        scenePath = Project::GetAssetPath(scenePath);
    }

    std::string finalPath = scenePath.string();

    if (m_Scene)
    {
        if (m_RuntimeStarted)
        {
            SceneScripting::OnRuntimeStop(m_Scene.get());
            m_Scene->OnRuntimeStop();
        }
        SceneScripting::Stop(m_Scene.get());
    }

    m_RuntimeStarted = false;
    m_IsSceneLoading = false;
    m_LoadingOverlayElapsed = 0.0f;

    m_Scene = std::make_shared<Scene>();

    // Native module loading removed in favor of ScriptEngine (C#)

    SceneSerializer serializer(m_Scene.get());
    if (serializer.Deserialize(finalPath))
    {
        m_Scene->GetSettings().ScenePath = finalPath;

        // Important: Update ScriptEngine context immediately
        ScriptEngine::Get().SetActiveScene(m_Scene.get());

        // Update camera aspect ratios for the new scene
        Window& window = Application::Get().GetWindow();
        m_Scene->OnViewportResize(window.GetWidth(), window.GetHeight());

        // Boost asset uploads for scene loading
        m_IsBoostingUploads = true;
        m_BoostUploadsTimer = 5.0f; // Boost for 5 seconds
        CH_CORE_INFO("RuntimeLayer: Boosting asset uploads for scene loading...");

        m_IsSceneLoading = true;
        m_LoadingOverlayElapsed = 0.0f;
        CH_CORE_INFO("RuntimeLayer: Scene loaded, waiting for async assets before runtime start.");
    }
    else
    {
        m_Scene = nullptr;
        m_RuntimeStarted = false;
        m_IsSceneLoading = false;
        m_LoadingOverlayElapsed = 0.0f;
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

    // Initialize Scripting for the loaded project
    ScriptEngine::Get().ReloadAssembly();

    ApplyWindowConfiguration();
    SetupBrandingAndIcon();

    auto& config = project->GetConfig();
    // Note: FPS control handled by engine main loop via Application class

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

    m_ProjectPath = Project::Discover(discoveryPath, Application::Get().GetSpecification().Name).string();

    if (m_ProjectPath.empty())
    {
        return false;
    }

    auto project = Project::Load(m_ProjectPath);
    if (!project)
    {
        return false;
    }

    // CRITICAL: Load engine shaders and resources immediately after project is resolved
    Renderer::LoadEngineResources();

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
    std::string resolved = AssetManager::Get().ResolvePath(config.IconPath);
    if (std::filesystem::exists(resolved))
    {
        iconPath = resolved;
    }

    if (iconPath.empty())
    {
        std::filesystem::path p = project->GetProjectDirectory() / config.IconPath;
        if (std::filesystem::exists(p))
        {
            iconPath = p;
        }
    }

    if (!iconPath.empty() && std::filesystem::exists(iconPath))
    {
        // Window icon loading handled by TextureLoader if needed
        // Icon path: iconPath.string()
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

    if (sceneToLoad.empty())
    {
        std::filesystem::path scenesDir = Project::GetAssetDirectory() / "scenes";
        if (std::filesystem::exists(scenesDir))
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(scenesDir))
            {
                if (entry.path().extension() == ".chscene")
                {
                    sceneToLoad = std::filesystem::relative(entry.path(), Project::GetAssetDirectory()).string();
                    break;
                }
            }
        }
    }

    if (!sceneToLoad.empty())
    {
        std::filesystem::path fullPath = Project::GetAssetPath(sceneToLoad);
        LoadScene(fullPath.string());
    }
}

std::optional<Camera3D> RuntimeLayer::GetActiveCamera()
{
    if (m_Scene)
    {
        return m_Scene->GetActiveCamera();
    }
    return std::nullopt;
}

void RuntimeLayer::EnsureRuntimeFramebuffer(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return;
    }

    if (!m_HDRFramebuffer)
    {
        FramebufferSpecification hdrSpec;
        hdrSpec.Width = width;
        hdrSpec.Height = height;
        hdrSpec.ColorFormat = FramebufferColorFormat::RGBA16F;
        m_HDRFramebuffer = Framebuffer::Create(hdrSpec);
        return;
    }

    const auto& spec = m_HDRFramebuffer->GetSpecification();
    if (spec.Width != width || spec.Height != height)
    {
        m_HDRFramebuffer->Resize(width, height);
    }
}

bool RuntimeLayer::IsSceneReadyToStart() const
{
    return !AssetManager::Get().HasBackgroundWork();
}

void RuntimeLayer::DrawLoadingOverlay()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.02f, 0.02f, 0.92f));

    if (ImGui::Begin("##RuntimeLoadingOverlay", nullptr, flags))
    {
        const size_t loadingCount = AssetManager::Get().GetLoadingAssetCount();
        const size_t pendingFinalizeCount = AssetManager::Get().GetPendingFinalizeCount();
        const size_t totalPending = loadingCount + pendingFinalizeCount;

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
} // namespace CHEngine
