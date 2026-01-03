#include "application.h"
#include <raylib.h>

namespace CH
{
Application *Application::s_Instance = nullptr;

Application::Application(const Config &config)
{
    s_Instance = this;
    InitWindow(config.Width, config.Height, config.Title.c_str());
    SetTargetFPS(60);
    m_Running = true;
}

Application::~Application()
{
    CloseWindow();
    s_Instance = nullptr;
}

void Application::Init(const Config &config)
{
    // For static access if needed, though constructor handles it
}

void Application::Shutdown()
{
    s_Instance->m_Running = false;
}

void Application::PushLayer(Layer *layer)
{
    s_Instance->m_LayerStack.PushLayer(layer);
    layer->OnAttach();
}

void Application::PushOverlay(Layer *overlay)
{
    s_Instance->m_LayerStack.PushOverlay(overlay);
    overlay->OnAttach();
}

void Application::BeginFrame()
{
    s_Instance->m_DeltaTime = GetFrameTime();

    for (Layer *layer : s_Instance->m_LayerStack)
        layer->OnUpdate(s_Instance->m_DeltaTime);

    BeginDrawing();
    ClearBackground(DARKGRAY);

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
