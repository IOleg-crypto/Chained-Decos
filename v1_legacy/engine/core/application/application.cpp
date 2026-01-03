#include "application.h"
#include "core/log.h"
#include "engine/core/input/input.h"
#include "engine/gui/gui_manager.h"
#include "engine/renderer/renderer.h"
#include "rlImGui.h"
#include <raylib.h>


namespace CHEngine
{
Application *Application::s_Instance = nullptr;

Application::Application(const std::string &name)
{
    CD_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

    WindowProps props;
    props.Title = name;
    m_Window = std::make_unique<Window>(props);
    m_Window->SetEventCallback(CD_BIND_EVENT_FN(Application::OnEvent));

    Renderer::Init();
    Renderer::SetBackgroundColor(SKYBLUE);
}

Application::~Application()
{
    Renderer::Shutdown();
}

void Application::PushLayer(Layer *layer)
{
    m_LayerStack.PushLayer(layer);
    layer->OnAttach();
}

void Application::PushOverlay(Layer *overlay)
{
    m_LayerStack.PushOverlay(overlay);
    overlay->OnAttach();
}

void Application::PopLayer(Layer *layer)
{
    m_LayerStack.PopLayer(layer);
    m_LayerDeletionQueue.push_back(layer);
}

void Application::PopOverlay(Layer *overlay)
{
    m_LayerStack.PopOverlay(overlay);
    m_LayerDeletionQueue.push_back(overlay);
}

void Application::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(CD_BIND_EVENT_FN(Application::OnWindowClose));
    dispatcher.Dispatch<WindowResizeEvent>(CD_BIND_EVENT_FN(Application::OnWindowResize));

    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
    {
        if (e.Handled)
            break;
        (*it)->OnEvent(e);
    }
}

void Application::Run()
{
    while (m_Running)
    {
        float time = (float)GetTime();
        float deltaTime = time - m_LastFrameTime;
        m_LastFrameTime = time;

        Input::Update();

        if (!m_Minimized)
        {
            for (Layer *layer : m_LayerStack)
                layer->OnUpdate(deltaTime);

            BeginDrawing();
            ClearBackground(Renderer::GetBackgroundColor());

            for (Layer *layer : m_LayerStack)
                layer->OnRender();

            rlImGuiBegin();
            for (Layer *layer : m_LayerStack)
                layer->OnImGuiRender();
            rlImGuiEnd();

            EndDrawing();
        }

        for (Layer *layer : m_LayerDeletionQueue)
            delete layer;
        m_LayerDeletionQueue.clear();

        m_Window->OnUpdate();
    }
}

bool Application::OnWindowClose(WindowCloseEvent &e)
{
    m_Running = false;
    return true;
}

bool Application::OnWindowResize(WindowResizeEvent &e)
{
    if (e.GetWidth() == 0 || e.GetHeight() == 0)
    {
        m_Minimized = true;
        return false;
    }

    m_Minimized = false;
    Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
    return false;
}

} // namespace CHEngine
