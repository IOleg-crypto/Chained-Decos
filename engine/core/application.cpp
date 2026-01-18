#include "application.h"
#include "engine/audio/audio_manager.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/core/thread_dispatcher.h"
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

Application::Application(const Config &config)
{
    CH_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

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
        // Project could also override fullscreen/fps if we wanted
    }

    m_Window = CreateScope<Window>(windowConfig);

    m_Running = true;

    Render::Init();
    SceneRender::Init();
    ThreadDispatcher::Init();
    Physics::Init();
    Assets::Init();
    AudioManager::Init();

    RegisterGameScripts();

    CH_CORE_INFO("Application Initialized: %s", config.Title);
}

Application::~Application()
{
    Assets::Shutdown();
    ThreadDispatcher::Shutdown();
    Physics::Shutdown();
    SceneRender::Shutdown();
    Render::Shutdown();
    s_Instance = nullptr;
}

void Application::Init(const Config &config)
{
    // For static access if needed, though constructor handles it
}

void Application::Shutdown()
{
    CH_CORE_INFO("Shutting down Engine...");
    AudioManager::Shutdown();
    s_Instance->m_Window.reset();
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
    Input::UpdateState();
    ThreadDispatcher::ExecuteMainThreadQueue();

    // Poll input
    Input::PollEvents(Application::OnEvent);

    s_Instance->m_DeltaTime = GetFrameTime();

    for (Layer *layer : s_Instance->m_LayerStack)
        layer->OnUpdate(s_Instance->m_DeltaTime);

    s_Instance->m_Window->BeginFrame();

    for (Layer *layer : s_Instance->m_LayerStack)
        layer->OnRender();

    for (Layer *layer : s_Instance->m_LayerStack)
        layer->OnImGuiRender();
}

void Application::EndFrame()
{
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

void Application::Run()
{
    while (!ShouldClose())
    {
        BeginFrame();
        EndFrame();
    }
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
    }
    else
    {
        CH_CORE_ERROR("Failed to load scene: {0}", path);
    }
}
} // namespace CHEngine
