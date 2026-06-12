#include "engine/core/application.h"
#include "engine/foundation/thread_pool.h"
#include "engine/core/imgui_layer.h"
#include "engine/core/platform.h"
#include "engine/core/profiler.h"

#include "engine/graphics/pipeline/renderer.h"
#include "engine/audio/audio.h"
#include "engine/physics/physics.h"
#include "engine/assets/asset_manager.h"
#include "engine/network/network_service.h"
#include "scripting/scriptengine.h"

namespace Chained
{
    Application* Application::s_Instance = nullptr;

std::filesystem::path Application::GetExecutableDirectory()
{
    return Platform::GetExecutableDirectory();
}

    Application::Application(const ApplicationSpecification& spec)
        : m_Specification(spec)
    {
        CH_ASSERT(!s_Instance);
        s_Instance = this;

        Log::Init();
        ThreadPool::Init();

        if (!m_Specification.WorkingDirectory.empty())
        {
            std::filesystem::current_path(m_Specification.WorkingDirectory);
        }

        // --- 1. Create Window ---
        if (!m_Specification.Headless)
        {
            WindowProperties props{ m_Specification.Name, m_Specification.WindowWidth, m_Specification.WindowHeight };
            props.VSync = m_Specification.VSync;
            props.Fullscreen = m_Specification.Fullscreen;
            props.Resizable = m_Specification.Resizable;
            props.IconPath = m_Specification.AppIcon;

            m_Window = std::unique_ptr<Window>(Window::Create(props));
            m_Window->SetEventCallback(CH_BIND_EVENT_FN(Application::OnEvent));
        }

        // --- 2. Initialize Subsystems ---
        AssetManager::Init();
        Renderer::Init(m_Specification.Headless);
        Audio::Init();
        Physics::Init();
        ScriptEngine::Init(true);
        // NetworkService::Init();

        if (m_Window)
        {
            Renderer::SetViewportSize(m_Window->GetWidth(), m_Window->GetHeight());
        }

        m_LayerStack = std::make_unique<LayerStack>();
        m_Running = true;

        if (!m_Specification.Headless)
        {
            auto imguiLayer = std::make_unique<ImGuiLayer>();
            m_ImGuiLayer = imguiLayer.get();
            PushOverlay(std::move(imguiLayer));
        }
    }

    Application::~Application()
    {
        m_LayerStack.reset();

        // Shutdown in reverse order
        ScriptEngine::Shutdown();
        Physics::Shutdown();
        Audio::Shutdown();
        // NetworkService::Shutdown();
        Renderer::Shutdown();
        AssetManager::Shutdown();
        ThreadPool::Shutdown();

        m_Window.reset();
        s_Instance = nullptr;
    }

void Application::Run()
{
    while (m_Running && (!m_Window || !m_Window->ShouldClose()))
    {
        float time = Platform::GetTime();
        m_Timer.DeltaTime = Timestep(time - m_Timer.LastFrameTime);
        m_Timer.LastFrameTime = time;

        if (m_Window && m_Window->GetWidth() > 0 && m_Window->GetHeight() > 0)
        {
            Audio::Get().Update(m_Timer.DeltaTime);
          

            // Fixed Update
            m_Timer.Accumulator += (float)m_Timer.DeltaTime;
            while (m_Timer.Accumulator >= m_Timer.FixedStepCount)
            {
                // Physics step is handled per-scene in Scene::OnUpdateRuntime

                for (auto& layer : *m_LayerStack)
                    layer->OnFixedUpdate(Timestep(m_Timer.FixedStepCount));

                m_Timer.Accumulator -= m_Timer.FixedStepCount;
            }

            // Шар оновлення логіки
            for (auto& layer : *m_LayerStack)
                layer->OnUpdate(m_Timer.DeltaTime);

            // Рендеринг кадру
            Profiler::BeginFrame();
            m_Window->BeginFrame();

            for (auto& layer : *m_LayerStack)
                layer->OnRender(m_Timer.DeltaTime);

            if (m_ImGuiLayer)
            {
                m_ImGuiLayer->Begin();
                for (auto& layer : *m_LayerStack)
                    layer->OnImGuiRender();
                m_ImGuiLayer->End();
            }

            m_Window->EndFrame();
            Profiler::EndFrame();
        }
    }
}

void Application::OnEvent(Event& e)
{
    if (e.GetEventType() == EventType::WindowResize)
    {
        auto& re = (WindowResizeEvent&)e;
        Renderer::SetViewportSize(re.GetWidth(), re.GetHeight());
    }

    for (auto it = m_LayerStack->rbegin(); it != m_LayerStack->rend(); ++it)
    {
        if (e.Handled) break;
        (*it)->OnEvent(e);
    }
}

void Application::PushLayer(std::unique_ptr<Layer> layer)
{
    Layer* raw = layer.get();
    m_LayerStack->PushLayer(std::move(layer));
    raw->OnAttach();
}

void Application::PushOverlay(std::unique_ptr<Layer> overlay)
{
    Layer* raw = overlay.get();
    m_LayerStack->PushOverlay(std::move(overlay));
    raw->OnAttach();
}


} // namespace Chained