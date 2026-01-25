#include "application.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/core/task_system.h"
#include "engine/physics/physics.h"
#include "engine/renderer/asset_manager.h"
#include "engine/renderer/render.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/script_registry.h"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <raylib.h>
#include <rlgl.h>

// GLFW
#define GLFW_INCLUDE_NONE
#include "external/glfw/include/GLFW/glfw3.h"

namespace CHEngine
{
Application *Application::s_Instance = nullptr;

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

    TaskSystem::Init();
    Render::Init();
    Physics::Init();
    Profiler::Init();
    Assets::Init();
    InitAudioDevice();
    if (IsAudioDeviceReady())
        CH_CORE_INFO("Audio Device Initialized Successfully");
    else
        CH_CORE_ERROR("Failed to initialize Audio Device!");

    // ImGui is now initialized in Window constructor

    RegisterGameScripts();

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
    Assets::Shutdown();
    Physics::Shutdown();
    Render::Shutdown();
    TaskSystem::Shutdown();

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
    Input::UpdateState();

    // Poll input
    Input::PollEvents(Application::OnEvent);

    s_Instance->m_DeltaTime = GetFrameTime();
    s_Instance->m_Window->BeginFrame();
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::EndFrame()
{
    CH_PROFILE_FUNCTION();

    // 1. Flush Raylib's internal drawing batch to the backbuffer
    // We do this manually so we can draw ImGui on top BEFORE the swap buffers call inside
    // EndDrawing()
    rlDrawRenderBatchActive();

    // 2. Render ImGui on top of the flushed Raylib content
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // 3. Update and Render additional Platform Windows (Viewports)
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    // 4. Finally call EndDrawing() which will perform the actual SwapBuffers()
    // Since we already called rlglDraw(), this will just handle the swap and timing
    s_Instance->m_Window->EndFrame();

    // 5. Handle deferred scene change
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
    // Optional: too verbose?
    // CH_CORE_TRACE("Event: %s", e.GetName());

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
        m_Window->PollEvents();

        float time = (float)GetTime();
        m_DeltaTime = time - m_LastFrameTime;
        m_LastFrameTime = time;

        Profiler::BeginFrame();
        Assets::Update(); // Synchronize GPU resources
        {
            CH_PROFILE_SCOPE("MainThread_Frame");

            if (!m_Minimized)
            {
                BeginFrame(); // Start ImGui frame before Update
                OnUpdate(m_DeltaTime);
                OnRender();
                EndFrame();
            }
        }
        Profiler::EndFrame();
    }
}

void Application::OnUpdate(float deltaTime)
{
    CH_PROFILE_FUNCTION();

    if (m_ActiveScene)
    {
        bool isSim = m_ActiveScene->IsSimulationRunning();
        Physics::Update(m_ActiveScene.get(), deltaTime, isSim);

        if (isSim)
            m_ActiveScene->OnUpdateRuntime(deltaTime);
    }

    for (auto layer : m_LayerStack)
    {
        if (layer->IsEnabled())
            layer->OnUpdate(deltaTime);
    }
}

void Application::OnRender()
{
    CH_PROFILE_FUNCTION();
    // BeginFrame() and EndFrame() moved to Run() to encapsulate Update

    // Render logic moved to layers and scene directly in orchestrated frame

    // Layers still need to render (mostly ImGui)
    for (auto layer : m_LayerStack)
    {
        if (layer->IsEnabled())
            layer->OnRender();
    }

    for (auto layer : m_LayerStack)
    {
        if (layer->IsEnabled())
            layer->OnImGuiRender();
    }
}

bool Application::IsRunning()
{
    return s_Instance->m_Running;
}

void Application::LoadScene(const std::string &path)
{
    CH_CORE_INFO("Loading scene: {0}", path);

    // 1. Shutdown current scene
    if (m_ActiveScene)
        m_ActiveScene->OnRuntimeStop();

    // 2. Load and deserialize new scene
    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();
    SceneSerializer serializer(newScene.get());
    if (serializer.Deserialize(path))
    {
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
