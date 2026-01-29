#include "application.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
// Removed redundant include: engine/core/task_system.h
#include "engine/physics/physics.h"
// Removed redundant include: engine/graphics/asset_manager.h
// Removed redundant include: engine/graphics/render.h
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "engine/graphics/visuals.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/script_registry.h"
#include "imgui.h"
#include "raylib.h"
#include "rlgl.h"

// GLFW
#define GLFW_INCLUDE_NONE
#include "external/glfw/include/glfw/glfw3.h"

namespace CHEngine
{
Application *Application::s_Instance = nullptr;
std::string Application::s_StartupScenePath = "";

Application::Application(const Config &config) : m_Config(config)
{
    CH_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;
}

Application::~Application()
{
    Shutdown();
    s_Instance = nullptr;
}

bool Application::Initialize(const Config &config)
{
    CH_CORE_INFO("Initializing Engine...");

    WindowConfig windowConfig;
    windowConfig.Title = config.Title;
    windowConfig.Width = config.Width;
    windowConfig.Height = config.Height;
    windowConfig.Fullscreen = config.Fullscreen;
    windowConfig.TargetFPS = config.TargetFPS;

    if (Project::GetActive())
    {
        auto &projConfig = Project::GetActive()->GetConfig();
        windowConfig.Width = projConfig.Window.Width;
        windowConfig.Height = projConfig.Window.Height;
        windowConfig.VSync = projConfig.Window.VSync;
        windowConfig.Resizable = projConfig.Window.Resizable;
    }

    m_Window = std::make_unique<Window>(windowConfig);
    m_Running = true;

    if (config.WindowIcon.data != nullptr)
    {
        m_Window->SetWindowIcon(config.WindowIcon);
    }
    Visuals::Init();
    Physics::Init();
    InitAudioDevice();
    if (IsAudioDeviceReady())
        CH_CORE_INFO("Audio Device Initialized Successfully");
    else
        CH_CORE_ERROR("Failed to initialize Audio Device!");

    // ImGui is now initialized in Window constructor

    CH_CORE_INFO("Application Initialized: {}", config.Title);

    // Auto-start runtime for standalone apps
    if (m_Config.Title != "Chained Editor")
    {
        if (m_ActiveScene)
            m_ActiveScene->OnRuntimeStart();
    }

    PostInitialize();
    return true;
}

void Application::Shutdown()
{
    if (!m_Running)
        return;

    CH_CORE_INFO("Shutting down Engine...");

    CloseAudioDevice();
    // ImGui shutdown is handled in Window destructor
    Physics::Shutdown();
    Visuals::Shutdown();
    m_Window.reset();
    m_Running = false;
    CH_CORE_INFO("Engine Shutdown Successfully.");
}

void Application::PushLayer(Layer *layer)
{
    CH_CORE_ASSERT(layer, "Layer is null!");
    s_Instance->m_LayerStack.PushLayer(layer);
    layer->OnAttach();
    CH_CORE_INFO("Layer Pushed: {}", layer->GetName());
}

void Application::PushOverlay(Layer *overlay)
{
    CH_CORE_ASSERT(overlay, "Overlay is null!");
    s_Instance->m_LayerStack.PushOverlay(overlay);
    overlay->OnAttach();
    CH_CORE_INFO("Overlay Pushed: {}", overlay->GetName());
}

void Application::BeginFrame()
{
    CH_PROFILE_FUNCTION();

    s_Instance->m_DeltaTime = GetFrameTime();
    s_Instance->m_Window->BeginFrame();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::EndFrame()
{
    rlDrawRenderBatchActive();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    s_Instance->m_Window->EndFrame();

    if (!s_Instance->m_NextScenePath.empty())
    {
        std::string nextPath = s_Instance->m_NextScenePath;
        s_Instance->m_NextScenePath.clear();
        s_Instance->LoadScene(nextPath);
    }
}

bool Application::ShouldClose()
{
    return !s_Instance->m_Running || s_Instance->m_Window->ShouldClose();
}

void Application::OnEvent(Event &e)
{
    for (auto it = s_Instance->m_LayerStack.rbegin(); it != s_Instance->m_LayerStack.rend(); ++it)
    {
        if (e.Handled)
            break;
        if ((*it)->IsEnabled())
            (*it)->OnEvent(e);
    }
}

void Application::Close()
{
    m_Running = false;
}

void Application::Run()
{
    while (m_Running && !m_Window->ShouldClose())
    {
        ProcessEvents();
        Simulate();
        Animate();
        Render();
    }
}

void Application::ProcessEvents()
{
    m_Window->PollEvents();

    // Hazel-style: Generate input events from Raylib here
    // 1. Keyboard
    int key = GetKeyPressed();
    while (key != 0)
    {
        KeyPressedEvent e(key, false);
        OnEvent(e);
        key = GetKeyPressed();
    }

    // Detect releases for a range of common keys (simplification for Raylib) or track them.
    // For now, checking the standard 512 keys range for state changes.
    // This ensures we get release events even if we didn't track them specifically in a vector.
    for (int k = 1; k < 512; k++)
    {
        if (::IsKeyReleased(k))
        {
            KeyReleasedEvent e(k);
            OnEvent(e);
        }
    }
    // 2. Mouse
    auto handleMouse = [&](int button)
    {
        if (::IsMouseButtonPressed(button))
        {
            MouseButtonPressedEvent e(button);
            OnEvent(e);
        }
        if (::IsMouseButtonReleased(button))
        {
            MouseButtonReleasedEvent e(button);
            OnEvent(e);
        }
    };
    handleMouse(MOUSE_BUTTON_LEFT);
    handleMouse(MOUSE_BUTTON_RIGHT);
    handleMouse(MOUSE_BUTTON_MIDDLE);

    Vector2 currentMousePos = ::GetMousePosition();
    static Vector2 lastMousePos = {0, 0};
    if (currentMousePos.x != lastMousePos.x || currentMousePos.y != lastMousePos.y)
    {
        MouseMovedEvent e(currentMousePos.x, currentMousePos.y);
        OnEvent(e);
        lastMousePos = currentMousePos;
    }

    float wheel = ::GetMouseWheelMove();
    if (wheel != 0)
    {
        MouseScrolledEvent e(0, wheel);
        OnEvent(e);
    }

    float time = (float)GetTime();
    m_DeltaTime = time - m_LastFrameTime;
    m_LastFrameTime = time;
}

void Application::Simulate()
{
    Profiler::BeginFrame();
    {
        CH_PROFILE_SCOPE("MainThread_Frame");
        if (!m_Minimized)
        {
            if (m_ActiveScene)
            {
                bool isSim = m_ActiveScene->IsSimulationRunning();
                Physics::Update(m_ActiveScene.get(), m_DeltaTime, isSim);
                if (isSim)
                    m_ActiveScene->OnUpdateRuntime(m_DeltaTime);
            }

            for (auto layer : m_LayerStack)
                if (layer->IsEnabled())
                    layer->OnUpdate(m_DeltaTime);
        }
    }
}

void Application::Animate()
{
    // Animations are currently handled in Scene::OnUpdateRuntime
}

void Application::OnRender()
{
}

void Application::SetWindowIcon(Image icon)
{
    if (m_Window)
        m_Window->SetWindowIcon(icon);
}

void Application::Render()
{
    if (m_Minimized)
        return;

    BeginFrame();

    for (auto layer : m_LayerStack)
        if (layer->IsEnabled())
            layer->OnRender();

    for (auto layer : m_LayerStack)
        if (layer->IsEnabled())
            layer->OnImGuiRender();

    EndFrame();
    Profiler::EndFrame();
}

bool Application::IsRunning()
{
    return s_Instance->m_Running;
}

void Application::LoadScene(const std::string &path)
{
    std::string pathStr = path;

    CH_CORE_INFO("Loading scene: {0}", pathStr);

    if (m_ActiveScene)
        m_ActiveScene->OnRuntimeStop();

    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();
    SceneSerializer serializer(newScene.get());
    if (serializer.Deserialize(pathStr))
    {
        newScene->SetScenePath(pathStr);
        m_ActiveScene = newScene;

        // Notify system that scene is opened
        SceneOpenedEvent e(path);
        Application::OnEvent(e);
    }
    else
    {
        CH_CORE_ERROR("Failed to load scene: {}", path);
    }
}
} // namespace CHEngine
