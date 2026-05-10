#include "application.h"
#include "engine/assets/asset_manager.h"
#include "engine/audio/audio.h"
#include "engine/core/imgui_layer.h"
#include "engine/core/input.h"
#include "engine/core/profiler.h"
#include "engine/core/thread_pool.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/graphics/pipeline/ui_renderer.h"
#include "engine/graphics/texture_system.h"
#include "engine/physics/physics_system.h"
#include "engine/scene/component_serializer.h"
#include "engine/scene/project.h"
#include "scripting/scriptengine.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <nfd.h>

#if defined(CH_PLATFORM_WINDOWS)
#include <windows.h>
#elif defined(CH_PLATFORM_LINUX)
#include <unistd.h>
#endif

namespace CHEngine
{
Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& spec)
    : m_Specification(spec)
{
    CH_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

    if (!m_Specification.WorkingDirectory.empty())
    {
        std::filesystem::current_path(m_Specification.WorkingDirectory);
    }

    // Discover engine root early as it's needed for system resource loading
    Project::DiscoverEngineRoot(std::filesystem::current_path());

    // 1. Boot Foundation (no GL)
    AddService<ThreadPool>();
    AddService<ComponentSerializer>();
    NFD_Init();

    auto resolver = std::make_shared<AssetPathResolver>();
    resolver->SetEngineRoot(Project::GetEngineRoot());
    auto registry = std::make_shared<AssetRegistry>();
    AddService<AssetManager>(resolver, registry);

    // 2. Initialize Window and OpenGL Context
    if (!m_Specification.Headless)
    {
        WindowProperties props;
        props.Title = m_Specification.Name;
        props.Width = m_Specification.WindowWidth;
        props.Height = m_Specification.WindowHeight;
        props.VSync = m_Specification.VSync;
        props.Fullscreen = m_Specification.Fullscreen;
        props.Resizable = m_Specification.Resizable;
        props.IconPath = m_Specification.AppIcon;

        m_Window = std::unique_ptr<Window>(Window::Create(props));
        m_Window->SetEventCallback(CH_BIND_EVENT_FN(Application::OnEvent));
    }

    // 3. Initialize Systems (Window/GL ready)
    auto& renderer = AddService<Renderer>();
    renderer.SetHeadless(m_Specification.Headless);
    if (m_Window)
    {
        renderer.SetViewportSize(m_Window->GetWidth(), m_Window->GetHeight());
    }
    AddService<TextureSystem>();
    AddService<Audio>();
    AddService<PhysicsSystem>();
    AddService<UIRenderer>();

    // Orchestrate Service Bootup via Template Method
    for (auto& svc : m_Services)
    {
        svc->Start();
    }

    // 4. Scripting — now a proper EngineService, added last so it shuts down first
    auto& scripting = AddService<ScriptEngine>(m_Specification.EnableScripting);
    scripting.Start();

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

    // Shutdown Services in reverse order (Conductor Shutdown)
    for (auto it = m_Services.rbegin(); it != m_Services.rend(); ++it)
    {
        (*it)->Stop();
    }
    m_Services.clear();

    m_Window.reset();

    ServiceLocator::Shutdown();
    NFD_Quit();

    s_Instance = nullptr;
}

void Application::Run()
{
    while (m_Running && (!m_Window || !m_Window->ShouldClose()))
    {
        float time = (float)glfwGetTime();
        m_Timer.DeltaTime = Timestep(time - m_Timer.LastFrameTime);
        m_Timer.LastFrameTime = time;

        // Start of frame: Prepare input state for transition detection, THEN poll new events
        Input::Update();
        Input::PollEvents();

        if (!m_Minimized)
        {
            // Conductor Update: Standard Services
            for (auto& svc : m_Services)
            {
                svc->Tick(m_Timer.DeltaTime);
            }

            // Fixed Update (Physics/Logic)
            m_Timer.Accumulator += (float)m_Timer.DeltaTime;
            while (m_Timer.Accumulator >= m_Timer.FixedStepCount)
            {
                for (auto& layer : *m_LayerStack)
                {
                    layer->OnFixedUpdate(Timestep(m_Timer.FixedStepCount));
                }
                m_Timer.Accumulator -= m_Timer.FixedStepCount;
            }

            // Game Layers Update
            for (auto& layer : *m_LayerStack)
            {
                layer->OnUpdate(m_Timer.DeltaTime);
            }

            // Frame Rendering
            if (m_Window)
            {
                Profiler::BeginFrame();

                m_Window->BeginFrame();
                for (auto& layer : *m_LayerStack)
                {
                    layer->OnRender(m_Timer.DeltaTime);
                }

                m_ImGuiLayer->Begin();
                for (auto& layer : *m_LayerStack)
                {
                    layer->OnImGuiRender();
                }
                m_ImGuiLayer->End();
                m_Window->EndFrame();

                Profiler::EndFrame();
            }
        }
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

void Application::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(CH_BIND_EVENT_FN(Application::OnWindowClose));
    dispatcher.Dispatch<WindowResizeEvent>(CH_BIND_EVENT_FN(Application::OnWindowResize));

    for (auto it = m_LayerStack->rbegin(); it != m_LayerStack->rend(); ++it)
    {
        if (e.Handled)
        {
            break;
        }
        (*it)->OnEvent(e);
    }
}

bool Application::OnWindowClose(WindowCloseEvent& e)
{
    m_Running = false;
    return true;
}

bool Application::OnWindowResize(WindowResizeEvent& e)
{
    if (e.GetWidth() == 0 || e.GetHeight() == 0)
    {
        m_Minimized = true;
        return false;
    }

    m_Minimized = false;
    m_Window->SetSizeDirect(e.GetWidth(), e.GetHeight());
    if (ServiceLocator::Has<Renderer>())
    {
        ServiceLocator::Get<Renderer>().SetViewportSize(e.GetWidth(), e.GetHeight());
    }
    return false;
}

std::filesystem::path Application::GetExecutableDirectory()
{
#if defined(CH_PLATFORM_WINDOWS)
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
#elif defined(CH_PLATFORM_LINUX)
    char path[1024];
    ssize_t count = readlink("/proc/self/exe", path, sizeof(path));
    return std::filesystem::path(std::string(path, (count > 0) ? count : 0)).parent_path();
#else
    return std::filesystem::current_path();
#endif
}
} // namespace CHEngine
