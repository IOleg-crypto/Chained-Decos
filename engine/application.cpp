#include "application.h"
#include "renderer.h"
#include <raylib.h>

namespace CH
{
Application *Application::s_Instance = nullptr;

Application::Application(const Config &config)
{
    CH_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

    InitWindow(config.Width, config.Height, config.Title.c_str());
    SetTargetFPS(60);
    m_Running = true;

    Renderer::Init();

    CH_CORE_INFO("Application Initialized: %s (%dx%d)", config.Title, config.Width, config.Height);
}

Application::~Application()
{
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
    CH_CORE_INFO("Application Shutdown requested");
    s_Instance->m_Running = false;
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
