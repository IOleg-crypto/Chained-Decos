#include "runtime_layer.h"
#include "engine/core/application.h"
#include "engine/core/window.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/renderer.h"
#include "engine/graphics/scene_renderer.h"
#include "engine/graphics/ui_renderer.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/scene_serializer.h"
#include "imgui.h"
#include "raymath.h"
#include "scripting/scene_scripting.h"
#include "scripting/scriptengine.h"
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
    m_ScriptEngine = std::make_unique<ScriptEngine>();
    m_ScriptEngine->Init();

    auto& assetManager = AssetManager::Get();
    if (assetManager.GetRootPath().empty())
    {
        assetManager.Initialize();
    }

    if (InitProject(m_ProjectPath))
    {
        // Initial scene/module load is handled by InitProject calling LoadInitialScene
    }

    ImGuiIO& io = ImGui::GetIO();
    float fontSize = 16.0f;

    // --- Default UI Font (Lato) ---
    std::string fontPath = assetManager.ResolvePath("resources/font/lato/lato-bold.ttf");
    if (std::filesystem::exists(fontPath))
    {
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
        CH_CORE_INFO("Runtime: Loaded engine font: {}", fontPath);
    }
    else
    {
        CH_CORE_WARN("Runtime: Engine font not found at {}. Using default ImGui font.", fontPath);
        io.Fonts->AddFontDefault();
    }

    // Ensure camera aspect ratio is correct on startup
    if (m_Scene)
    {
        Window& window = Application::Get().GetWindow();
        m_Scene->OnViewportResize(window.GetWidth(), window.GetHeight());
    }
}

void RuntimeLayer::OnDetach()
{
    if (m_Scene)
    {
        SceneScripting::Stop(m_Scene.get());
        m_Scene->OnRuntimeStop();
    }
    m_ScriptEngine.reset();
}

void RuntimeLayer::OnUpdate(Timestep ts)
{
    std::string pendingPath = ScriptEngine::Get().ConsumeRequestedScene();
    if (!pendingPath.empty())
    {
        LoadScene(pendingPath);
    }

    if (m_Scene)
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
            AssetManager::Get().SetMaxUploadsPerFrame(50); // Reset to default
            CH_CORE_INFO("RuntimeLayer: Asset upload boost finished, reset limit to 50.");
        }
    }
}

void RuntimeLayer::OnRender(Timestep ts)
{
    if (!m_Scene)
    {
        ::ClearBackground(BLACK);
        return;
    }

    Color bgColor = BLACK;
    bool clearBackground = true;

    if (m_Scene->GetSettings().Environment)
    {
        auto& env = m_Scene->GetSettings().Environment->GetSettings();
        if (env.Fog.Enabled)
        {
            bgColor = env.Fog.FogColor;
        }
        else if (!env.Skybox.TexturePath.empty())
        {
            clearBackground = false; // Skybox will draw over everything, no need to clear to a solid color
        }
    }

    if (clearBackground)
    {
        ::ClearBackground(bgColor);
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

        m_SceneRenderer->RenderScene(m_Scene.get(), camera.value(), nearClip, farClip, ts);
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
            ImVec2 canvasPos = ImGui::GetCursorScreenPos();
            ImVec2 canvasSize = ImGui::GetContentRegionAvail();
            UIRenderer::Get().DrawCanvas(m_Scene.get(), canvasPos, canvasSize, false);
            SceneScripting::RenderUI(m_Scene.get());
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    }
}

void RuntimeLayer::OnEvent(Event& e)
{
    if (m_Scene)
    {
        SceneScripting::DispatchEvent(m_Scene.get(), e);
        m_Scene->OnEvent(e);
    }

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<SceneChangeRequestEvent>([this](auto& ev) {
        ScriptEngine::Get().RequestLoadScene(ev.GetPath());
        return true;
    });
}

void RuntimeLayer::LoadScene(const std::string& path)
{
    std::filesystem::path scenePath = path;
    if (scenePath.is_relative() && Project::GetActive())
    {
        scenePath = Project::GetAssetPath(path);
    }

    std::string finalPath = scenePath.string();

    if (m_Scene)
    {
        SceneScripting::Stop(m_Scene.get());
        m_Scene->OnRuntimeStop();
    }

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
        AssetManager::Get().SetMaxUploadsPerFrame(2000); // Aggressive upload boost for loading
        CH_CORE_INFO("RuntimeLayer: Boosting asset uploads for scene loading...");

        m_Scene->OnRuntimeStart();
    }
    else
    {
        CH_CORE_ERROR("RuntimeLayer: Failed to load scene: {}", finalPath);
        m_Scene = nullptr;
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
    ::SetTargetFPS((int)config.Animation.TargetFPS > 0 ? (int)config.Animation.TargetFPS : 60);

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
        CH_CORE_ERROR("Runtime: Failed to load project at {}", m_ProjectPath);
        return false;
    }

    // CRITICAL: Load engine shaders and resources immediately after project is resolved
    Renderer::LoadEngineResources(AssetManager::Get());

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
    if (width != config.Window.Width || height != config.Window.Height)
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
        Image icon = LoadImage(iconPath.string().c_str());
        if (icon.data != nullptr)
        {
            if (icon.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
            {
                ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            }

            window.SetWindowIcon(icon);
            UnloadImage(icon);
        }
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
} // namespace CHEngine
