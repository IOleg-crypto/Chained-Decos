#include "application.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/core/main_thread_queue.h"
#include "engine/core/profiler.h"
#include "engine/physics/physics.h"
#include "engine/renderer/asset_manager.h"
#include "engine/renderer/render.h"
#include "engine/renderer/scene_render.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_serializer.h"
#include "engine/scene/script_registry.h"
#include <raylib.h>
#include <rlImGui.h>

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
    SetTraceLogLevel(LOG_WARNING);

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

    m_Window = CreateScope<Window>(windowConfig);
    m_Running = true;

    Render::Init();
    SceneRender::Init();
    Physics::Init();
    Profiler::Init();
    Assets::Init();
    InitAudioDevice();
    if (IsAudioDeviceReady())
        CH_CORE_INFO("Audio Device Initialized Successfully");
    else
        CH_CORE_ERROR("Failed to initialize Audio Device!");

    RegisterGameScripts();

    CH_CORE_INFO("Application Initialized: %s", config.Title);

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
    Assets::Shutdown();
    Physics::Shutdown();
    SceneRender::Shutdown();
    Render::Shutdown();

    m_Window.reset();
    m_Running = false;
    CH_CORE_INFO("Engine Shutdown Successfully.");
}

void Application::PushLayer(Layer *layer)
{
    CH_CORE_ASSERT(layer, "Layer is null!");
    s_Instance->m_LayerStack.PushLayer(layer);
    layer->OnAttach();
    CH_CORE_INFO("Layer Pushed: %s", layer->GetName());
}

void Application::PushOverlay(Layer *overlay)
{
    CH_CORE_ASSERT(overlay, "Overlay is null!");
    s_Instance->m_LayerStack.PushOverlay(overlay);
    overlay->OnAttach();
    CH_CORE_INFO("Overlay Pushed: %s", overlay->GetName());
}

void Application::BeginFrame()
{
    CH_PROFILE_FUNCTION();
    Input::UpdateState();
    MainThread::ProcessAll();

    // Poll input
    Input::PollEvents(Application::OnEvent);

    s_Instance->m_DeltaTime = GetFrameTime();
    s_Instance->m_Window->BeginFrame();
}

void Application::EndFrame()
{
    CH_PROFILE_FUNCTION();
    s_Instance->m_Window->EndFrame();
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
        (*it)->OnEvent(e);
    }
}

void Application::Close()
{
    m_Running = false;
}

void Application::Run()
{
    m_SimulationThread = std::thread([this]() { UpdateSimulation(); });

    while (m_Running && !m_Window->ShouldClose())
    {
        Profiler::BeginFrame();
        {
            CH_PROFILE_SCOPE("MainThread_Frame");

            if (!m_Minimized)
            {
                // Logic/Sim is running on the other thread.
                // We just render using the LATEST snapshot available.
                OnRender();
            }
        }
        Profiler::EndFrame();
        m_Window->PollEvents();
    }

    m_Running = false; // Signal sim thread to stop
    if (m_SimulationThread.joinable())
        m_SimulationThread.join();
}

void Application::UpdateSimulation()
{
    CH_CORE_INFO("Simulation Thread Started");

    while (m_Running)
    {
        CH_PROFILE_SCOPE("SimulationCycle");

        float time = (float)GetTime();
        m_DeltaTime = time - m_LastFrameTime;
        m_LastFrameTime = time;

        if (!m_Minimized)
        {
            {
                std::lock_guard<std::mutex> lock(m_SimMutex);

                // Fixed Update Loop (Internal Physics Sub-stepping)
                m_FixedTimeAccumulator += m_DeltaTime;
                while (m_FixedTimeAccumulator >= m_FixedStep)
                {
                    // Update Previous State for interpolation
                    if (m_ActiveScene)
                    {
                        bool isSim = m_ActiveScene->IsSimulationRunning();
                        static int simReportTimer = 0;
                        if (simReportTimer++ % 120 == 0)
                        {
                            // CH_CORE_TRACE("Simulation: Loop running. SimActive={0}", isSim);
                        }

                        auto &reg = m_ActiveScene->GetRegistry();
                        reg.view<TransformComponent>().each(
                            [](auto &transform)
                            {
                                transform.PrevTranslation = transform.Translation;
                                transform.PrevRotationQuat = transform.RotationQuat;
                                transform.PrevScale = transform.Scale;
                            });

                        // Internal Physics Step (Integration only during simulation)
                        Physics::Update(m_ActiveScene.get(), m_FixedStep, isSim);
                    }

                    m_FixedTimeAccumulator -= m_FixedStep;
                }

                // Unified Update (Game Logic, Scripts, etc.)
                OnUpdate(m_DeltaTime);

                // Take Snapshot for the renderer
                if (m_ActiveScene)
                {
                    // FIXME: We need a way to determine the active camera from the scene
                    // For now, we take a "snapshot" without knowing which camera will be used,
                    // but we pass a default one. The actual camera will be set by the
                    // Layer::OnRender.
                    Camera3D camera = {0};

                    float alpha = m_FixedTimeAccumulator / m_FixedStep;
                    SceneRender::CreateSnapshot(m_ActiveScene.get(), camera,
                                                m_RenderStates[m_SimBufferIndex], alpha);

                    // Swap Sim buffer with Pending buffer
                    std::swap(m_SimBufferIndex, m_PendingBufferIndex);
                    m_NewStateAvailable = true;
                }
            }
        }

        // Avoid pegged CPU if minimized or no work
        if (m_DeltaTime < 0.001f)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    CH_CORE_INFO("Simulation Thread Stopped");
}

void Application::OnUpdate(float deltaTime)
{
    CH_PROFILE_FUNCTION();
    for (auto layer : m_LayerStack)
        layer->OnUpdate(deltaTime);
}

void Application::OnRender()
{
    CH_PROFILE_FUNCTION();

    // Check for NEW snapshot
    if (m_NewStateAvailable)
    {
        std::lock_guard<std::mutex> lock(m_SimMutex);
        std::swap(m_RenderBufferIndex, m_PendingBufferIndex);
        m_NewStateAvailable = false;
    }

    BeginFrame();

    // Render using the SNAPSHOT
    SceneRender::SubmitScene(m_RenderStates[m_RenderBufferIndex]);

    // Layers still need to render (mostly ImGui)
    for (auto layer : m_LayerStack)
        layer->OnRender();

    for (auto layer : m_LayerStack)
        layer->OnImGuiRender();

    EndFrame();
}

bool Application::IsRunning()
{
    return s_Instance->m_Running;
}

void Application::LoadScene(const std::string &path)
{
    Ref<Scene> newScene = CreateRef<Scene>();
    SceneSerializer serializer(newScene.get());
    if (serializer.Deserialize(path))
    {
        m_ActiveScene = newScene;
        CH_CORE_INFO("Scene Loaded: {0}", path);

        // If we're not an editor, we should start the runtime automatically for the loaded scene
        if (m_Config.Title != "Chained Editor") // Simple check for now, can be improved
        {
            m_ActiveScene->OnRuntimeStart();
        }
    }
    else
    {
        CH_CORE_ERROR("Failed to load scene: {0}", path);
    }
}
} // namespace CHEngine
