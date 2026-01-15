#include "application.h"
#include "engine/audio/audio_manager.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/core/thread_dispatcher.h"
#include "engine/physics/physics.h"
#include "engine/renderer/asset_manager.h"
#include "engine/renderer/render.h"
#include "engine/renderer/scene_render.h"
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

    // Log::Init(); // Removed
    CH_CORE_INFO("Initializing Engine..."); // Added
    SetTraceLogLevel(LOG_WARNING);          // Reduce spam
    InitWindow(config.Width, config.Height, config.Title.c_str());
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // Prevent ESC from closing the app
    rlImGuiSetup(true);   // Added
    m_Running = true;

    Render::Init();
    SceneRender::Init();
    ThreadDispatcher::Init();
    Physics::Init();
    AssetManager::Init();
    AudioManager::Init(); // Added

    // Register Default Input Actions
    Input::RegisterAction("Jump", KEY_SPACE);
    Input::RegisterAction("Teleport", KEY_F);

    // Register Game-Specific Scripts
    RegisterGameScripts();

    CH_CORE_INFO("Application Initialized: %s (%dx%d)", config.Title, config.Width, config.Height);
}

Application::~Application()
{
    AssetManager::Shutdown();
    ThreadDispatcher::Shutdown();
    Physics::Shutdown();
    SceneRender::Shutdown();
    Render::Shutdown();
    CloseWindow();
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
    rlImGuiShutdown();
    CloseWindow();

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
    // Clear per-frame input state
    Input::UpdateState();

    ThreadDispatcher::ExecuteMainThreadQueue();

    PollEvents();

    s_Instance->m_DeltaTime = GetFrameTime();

    for (Layer *layer : s_Instance->m_LayerStack)
        layer->OnUpdate(s_Instance->m_DeltaTime);

    BeginDrawing();
    ClearBackground(DARKGRAY);

    for (Layer *layer : s_Instance->m_LayerStack)
        layer->OnRender();

    for (Layer *layer : s_Instance->m_LayerStack)
        layer->OnImGuiRender();
}

void Application::EndFrame()
{
    EndDrawing();
}

void Application::PollEvents()
{
    Input::PollEvents(Application::OnEvent);
}

bool Application::ShouldClose()
{
    return !s_Instance->m_Running || WindowShouldClose();
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

float Application::GetDeltaTime()
{
    return s_Instance->m_DeltaTime;
}
} // namespace CHEngine
