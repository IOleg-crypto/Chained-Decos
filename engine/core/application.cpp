#include "application.h"
#include "engine/audio/audio_manager.h" // Added
#include "engine/core/log.h"            // Added
#include "engine/physics/physics.h"
#include "engine/renderer/asset_manager.h"
#include "engine/renderer/renderer.h"
#include <raylib.h>
#include <rlImGui.h> // Added

namespace CH
{
Application *Application::s_Instance = nullptr;

Application::Application(const Config &config)
{
    CH_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

    // Log::Init(); // Removed
    CH_CORE_INFO("Initializing Engine..."); // Added

    InitWindow(config.Width, config.Height, config.Title.c_str());
    SetTargetFPS(60);
    rlImGuiSetup(true); // Added
    m_Running = true;

    Renderer::Init();
    Physics::Init();
    AssetManager::Init();
    AudioManager::Init(); // Added

    CH_CORE_INFO("Application Initialized: %s (%dx%d)", config.Title, config.Width, config.Height);
}

Application::~Application()
{
    AssetManager::Shutdown();
    Physics::Shutdown();
    Renderer::Shutdown();
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
    s_Instance->m_DeltaTime = GetFrameTime();

    for (Layer *layer : s_Instance->m_LayerStack)
        layer->OnUpdate(s_Instance->m_DeltaTime);

    // Update Physics (Scene based)
    // For now, we only update systems if we have an active scene
    // This might be better handled within a specific scene-owning layer
    // but for foundation we put it here or in EditorLayer.
    // However, Application doesn't know about ActiveScene yet (it's in EditorLayer).
    // So let's handle Physics update in the Layer that owns the scene.

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
} // namespace CH
