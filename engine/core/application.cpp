#include "application.h"
#include "engine/audio/audio.h"
#include "engine/core/assert.h"
#include "engine/core/imgui_layer.h"
#include "engine/core/input.h"
#include "engine/core/layer.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/core/thread_pool.h"
#include "engine/core/window.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/font_asset.h"
#include "engine/graphics/renderer.h"
#include "engine/physics/physics.h"
#include "engine/scene/component_serializer.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/scene_events.h"
#include "rlgl.h"

#include <algorithm>
#include <filesystem>
#include <ranges>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include "engine/graphics/render_command.h"
#include "engine/graphics/renderer.h"
#include "engine/graphics/renderer2d.h"
#include <GLFW/glfw3.h>

namespace CHEngine
{
Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& specification)
    : m_Specification(specification)
{
    CH_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

    if (!m_Specification.WorkingDirectory.empty())
    {
        std::filesystem::current_path(m_Specification.WorkingDirectory);
    }

    // --- Window Setup ---
    WindowProperties windowProps;
    windowProps.Title = m_Specification.Name;

    // ImGui Ini path setup
    std::string iniName = m_Specification.Name;
    std::replace(iniName.begin(), iniName.end(), ' ', '_');
    std::transform(iniName.begin(), iniName.end(), iniName.begin(), ::tolower);

#ifdef PROJECT_ROOT_DIR
    windowProps.ImGuiConfigurationPath = std::string(PROJECT_ROOT_DIR) + "/imgui_" + iniName + ".ini";
#else
    windowProps.ImGuiConfigurationPath = "imgui_" + iniName + ".ini";
#endif

    // --- System Initialization ---
    // Systems are singletons and manage their own lifetimes
    m_ThreadPool = new ThreadPool();
    if (!m_Specification.Headless)
    {
        m_Window = Window::Create(windowProps);
    }
    
    m_Renderer = new Renderer();
    m_Audio = new Audio();
    m_PhysicsSystem = new PhysicsSystem();
    m_ComponentSerializer = new ComponentSerializer();

    m_LayerStack = std::make_unique<LayerStack>();
    m_Running = true;

    // --- Core Systems Initialization ---
    m_ComponentSerializer->Initialize();
    m_Renderer->Init();
    m_PhysicsSystem->Init();
    m_PhysicsSystem->Init();
    m_Audio->Init();
    if (IsAudioDeviceReady())
    {
        CH_CORE_INFO("Audio Device Initialized Successfully");
    }
    else
    {
        CH_CORE_ERROR("Failed to initialize Audio Device!");
    }

    // ImGui Layer setup (always needed for Editor/Debugging)
    if (!m_Specification.Headless)
    {
        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);
    }

    CH_CORE_INFO("Application Initialized: {}", m_Specification.Name);
}

Application::~Application()
{
    CH_CORE_INFO("Shutting down Application...");
    
    
    if (m_Audio) m_Audio->Shutdown();
    if (m_PhysicsSystem) m_PhysicsSystem->Shutdown();
    if (m_Renderer) m_Renderer->Shutdown();
    
    // Cleanup subsystem instances safely using local pointers
    
    delete m_Audio;
    delete m_PhysicsSystem;
    delete m_Renderer;
    delete m_ThreadPool;
    delete m_ComponentSerializer;

    m_LayerStack->Shutdown();
    m_Window.reset();

    s_Instance = nullptr;
    CH_CORE_INFO("Engine Shutdown Successfully.");
}

void Application::PushLayer(Layer* layer)
{
    CH_CORE_ASSERT(layer, "Layer is null!");
    m_LayerStack->PushLayer(layer);
    layer->OnAttach();
    CH_CORE_INFO("Layer Attached: {}", layer->GetName());
}

void Application::PushOverlay(Layer* overlay)
{
    CH_CORE_ASSERT(overlay, "Overlay is null!");
    m_LayerStack->PushOverlay(overlay);
    overlay->OnAttach();
    CH_CORE_INFO("Overlay Attached: {}", overlay->GetName());
}

void Application::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(CH_BIND_EVENT_FN(Application::OnWindowClose));
    dispatcher.Dispatch<WindowResizeEvent>(CH_BIND_EVENT_FN(Application::OnWindowResize));

    // Propagate events from top to bottom (overlays first)
    // We use a copy of the layer stack to avoid iterator invalidation if a layer is removed during event handling
    auto layers = m_LayerStack->GetLayers();
    for (auto it = layers.rbegin(); it != layers.rend(); ++it)
    {
        if (e.Handled)
            break;
        if ((*it)->IsEnabled())
        {
            (*it)->OnEvent(e);
        }
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
        CH_CORE_WARN("Window minimized or dimensions are zero ({}x{})", e.GetWidth(), e.GetHeight());
        return false;
    }

    m_Minimized = false;
    m_Window->SetSizeDirect(e.GetWidth(), e.GetHeight());
    Renderer::Get().SetViewport(0, 0, e.GetWidth(), e.GetHeight());
    CH_CORE_INFO("Window resized to {}x{}", e.GetWidth(), e.GetHeight());

    return false;
}

void Application::SubmitToMainThread(const std::function<void()>& function)
{
    std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);
    m_MainThreadQueue.emplace_back(function);
}

void Application::ExecuteMainThreadQueue()
{
    std::vector<std::function<void()>> localQueue;
    {
        std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);
        localQueue = std::move(m_MainThreadQueue);
    }

    for (auto& func : localQueue)
    {
        func();
    }
}

void Application::Run()
{
    ::SetExitKey(NULL);

    while (m_Running && !m_Window->ShouldClose())
    {
        CH_PROFILE_FUNCTION();

        ExecuteMainThreadQueue();

        // 1. Time Tracking
        Timestep time = (float)GetTime();
        m_DeltaTime = Timestep(time - m_LastFrameTime);
        m_LastFrameTime = time;

        // 2. Input Polling
        Input::PollEvents();

        // 3. Core Systems Update
        if (auto project = Project::GetActive())
        {
            AssetManager::Get().Update();
        }

        // 4. Layers Update & Rendering
        Profiler::BeginFrame();
        {
            CH_PROFILE_SCOPE("MainThread_Frame");

            if (!m_Minimized)
            {
                // Logic/Simulation
                for (auto layer : *m_LayerStack)
                {
                    if (layer->IsEnabled())
                    {
                        layer->OnUpdate(m_DeltaTime);
                    }
                }

                // Rendering
                m_Window->BeginFrame();

                for (auto layer : *m_LayerStack)
                {
                    if (layer->IsEnabled())
                    {
                        layer->OnRender(m_DeltaTime);
                    }
                }

                // ImGui
                m_ImGuiLayer->Begin();
                for (auto layer : *m_LayerStack)
                {
                    if (layer->IsEnabled())
                    {
                        layer->OnImGuiRender();
                    }
                }
                m_ImGuiLayer->End();

                m_Window->EndFrame();
            }
        }
        Profiler::EndFrame();
    }
}

Window& Application::GetWindow()
{
    CH_CORE_ASSERT(m_Window, "Window is null (Headless mode?)");
    return *m_Window;
}

LayerStack& Application::GetLayerStack()
{
    return *m_LayerStack;
}

} // namespace CHEngine
