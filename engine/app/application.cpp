#include "engine/app/application.h"
#include "engine/core/platform.h"
#include "engine/core/profiler.h"
#include "engine/common/thread_pool.h"
#include "engine/imgui/imgui_layer.h"
#include "engine/core/events/window_events.h"
#include "engine/assets/asset_manager.h"
#include "engine/core/service_locator.h"
#include "engine/audio/audio.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/ui/widget_renderer.h"
#include "engine/graphics/pipeline/debug_renderer.h"
#include "engine/physics/physics.h"
#include "engine/scene/component_registry.h"
#include "engine/platform/dialogs/dialogs.h"
#include "engine/project/project.h"

#include "scripting/scriptengine.h"
#include "engine/core/input.h"

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
    Core::Input::Init();
    Dialogs::Init();
    ComponentRegistry::RegisterEngineComponents();

    unsigned int threads = std::thread::hardware_concurrency();
    if (threads == 0) threads = 1;
    unsigned int workerCount = (threads > 1) ? (threads - 1) : 1;

    if (!m_Specification.WorkingDirectory.empty())
    {
        std::filesystem::current_path(m_Specification.WorkingDirectory);
    }

    // --- 1. Create Window ---
    if (!m_Specification.Headless)
    {
        WindowProperties props{m_Specification.Name, m_Specification.WindowWidth, m_Specification.WindowHeight};
        props.VSync = m_Specification.VSync;
        props.Fullscreen = m_Specification.Fullscreen;
        props.Resizable = m_Specification.Resizable;
        props.IconPath = m_Specification.AppIcon;

        m_Window = std::unique_ptr<Window>(Window::Create(props));
        m_Window->SetEventCallback(CH_BIND_EVENT_FN(Application::OnEvent));
    }

    ServiceLocator::Provide<ThreadPool>(new ThreadPool(workerCount));
    ServiceLocator::Provide<AssetManager>(new AssetManager());

    // Set engine root BEFORE any other modules (like Renderer) try to load assets!
    if (!m_Specification.EngineRoot.empty())
    {
        ServiceLocator::Get<AssetManager>()->SetEngineRoot(m_Specification.EngineRoot);
        CH_CORE_INFO("AssetManager: Engine root set to '{}'", m_Specification.EngineRoot.string());
    }

    if (!m_Specification.Headless)
    {
        ServiceLocator::Provide<Renderer>(new Renderer());
        ServiceLocator::Provide<WidgetRenderer>(new WidgetRenderer());
        ServiceLocator::Provide<DebugRenderer>(new DebugRenderer());
    }
    ServiceLocator::Provide<Audio>(new Audio());
    ServiceLocator::Provide<Physics>(new Physics());
    ServiceLocator::Provide<ScriptEngine>(new ScriptEngine(m_Specification.EnableScripting));

    ServiceLocator::InitializeModule();
    ServiceLocator::Lock();


    if (m_Window)
    {
        ServiceLocator::Get<Renderer>()->SetViewportSize(m_Window->GetWidth(), m_Window->GetHeight());
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

    // WARNING: OpenGL resources held by the Project (like Environment maps)
    // MUST be released before the OpenGL context is destroyed (via m_Window.reset()).
    // Setting Active Project to nullptr invokes destructors cleanly.
    Project::SetActive(nullptr);

    ServiceLocator::Shutdown();
    Dialogs::Shutdown();
    Core::Input::Shutdown();
    // NOTE: ThreadPool is now an EngineModule, so its Shutdown() is handled natively by ServiceLocator::Shutdown()
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

        Profiler::BeginFrame();

        if (m_Window)
        {
            m_Window->BeginFrame();
        }

        if (m_Window && m_Window->GetWidth() > 0 && m_Window->GetHeight() > 0)
        {
            // 1. Systems Update
            ServiceLocator::Get<Audio>()->Update(m_Timer.DeltaTime);
            ServiceLocator::Get<AssetManager>()->Update(m_Timer.DeltaTime);

            // 2. Fixed Update
            m_Timer.Accumulator += (float)m_Timer.DeltaTime;
            while (m_Timer.Accumulator >= m_Timer.FixedStepCount)
            {
                for (auto& layer : *m_LayerStack)
                {
                    layer->OnFixedUpdate(Timestep(m_Timer.FixedStepCount));
                }

                m_Timer.Accumulator -= m_Timer.FixedStepCount;
            }

            // 3. Logic Update
            for (auto& layer : *m_LayerStack)
            {
                layer->OnUpdate(m_Timer.DeltaTime);
            }

            // 4. Input Backup
            Core::Input::Update(m_Timer.DeltaTime);

            // 5. Rendering

            for (auto& layer : *m_LayerStack)
            {
                layer->OnRender(m_Timer.DeltaTime);
            }

            if (m_ImGuiLayer)
            {
                m_ImGuiLayer->Begin();
                for (auto& layer : *m_LayerStack)
                {
                    layer->OnImGuiRender();
                }
                m_ImGuiLayer->End();
            }

            m_Window->EndFrame();
        }
        
        Profiler::EndFrame();
    }
}

void Application::OnEvent(Event& e)
{
    if (e.GetEventType() == EventType::WindowResize)
    {
        auto& re = (WindowResizeEvent&)e;
        ServiceLocator::Get<Renderer>()->SetViewportSize(re.GetWidth(), re.GetHeight());
    }

    for (auto it = m_LayerStack->rbegin(); it != m_LayerStack->rend(); ++it)
    {
        if (e.Handled)
        {
            break;
        }
        (*it)->OnEvent(e);
    }
}

void Application::PushLayer(std::unique_ptr<Layer> layer) const {
    Layer* raw = layer.get();
    m_LayerStack->PushLayer(std::move(layer));
    raw->OnAttach();
}

void Application::PushOverlay(std::unique_ptr<Layer> overlay) const {
    Layer* raw = overlay.get();
    m_LayerStack->PushOverlay(std::move(overlay));
    raw->OnAttach();
}

} // namespace Chained