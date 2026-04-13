#include "application.h"
#if CH_PLATFORM_WINDOWS
#include <windows.h>
#elif CH_PLATFORM_LINUX
#include <unistd.h>
#endif

#include "engine/audio/audio.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/ch_assert.h"
#include "engine/core/imgui_layer.h"
#include "engine/core/input.h"
#include "engine/core/layer.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/graphics/pipeline/renderer.h"
#include "engine/physics/physics.h"
#include "engine/scene/component_serializer.h"
#include "engine/scene/project.h"
#include "scripting/scriptengine.h"
#include <nfd.h>

#include <algorithm>
#include <filesystem>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include "engine/graphics/pipeline/ui_renderer.h"
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
    windowProps.Width = m_Specification.WindowWidth;
    windowProps.Height = m_Specification.WindowHeight;
    windowProps.VSync = m_Specification.VSync;
    windowProps.Fullscreen = m_Specification.Fullscreen;
    windowProps.Resizable = m_Specification.Resizable;
    windowProps.IconPath = m_Specification.AppIcon;

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
    if (!m_Specification.Headless)
    {
        m_Window = std::unique_ptr<Window>(Window::Create(windowProps));
#ifdef PROJECT_ROOT_DIR
        Project::SetEngineRoot(PROJECT_ROOT_DIR);
#endif
    }
    Renderer::Init();
    UIRenderer::Init();
    Physics::Init();
    ComponentSerializer::Init();
    ScriptEngine::Init();

    m_LayerStack = std::make_unique<LayerStack>();
    m_Running = true;

    // --- Core Systems Post-Initialization ---
    // Note: Systems' Init() already called InternalInit().
    // We only need to Push layers andoverlays.

    // ImGui Layer setup (always needed for Editor/Debugging)
    if (!m_Specification.Headless)
    {
        NFD_Init();
        auto imguiLayer = std::make_unique<ImGuiLayer>();
        m_ImGuiLayer = imguiLayer.get();
        PushOverlay(std::move(imguiLayer));
    }

    CH_CORE_INFO("Application Initialized: {}", m_Specification.Name);
}

Application::~Application()
{
    CH_CORE_INFO("Shutting down Application...");

    m_LayerStack.reset();

    ScriptEngine::Shutdown();
    ComponentSerializer::Shutdown();
    Physics::Shutdown();
    UIRenderer::Shutdown();
    Renderer::Shutdown();

    m_Window.reset();

    if (!m_Specification.Headless)
    {
        NFD_Quit();
    }

    s_Instance = nullptr;
    CH_CORE_INFO("Engine Shutdown Successfully.");
}

void Application::PushLayer(std::unique_ptr<Layer> layer)
{
    CH_CORE_ASSERT(layer, "Layer is null!");
    Layer* rawLayer = layer.get();
    m_LayerStack->PushLayer(std::move(layer));
    rawLayer->OnAttach();
    CH_CORE_INFO("Layer Attached: {}", rawLayer->GetName());
}

void Application::PushLayer(Layer* layer)
{
    PushLayer(std::unique_ptr<Layer>(layer));
}

void Application::PushOverlay(std::unique_ptr<Layer> overlay)
{
    CH_CORE_ASSERT(overlay, "Overlay is null!");
    Layer* rawOverlay = overlay.get();
    m_LayerStack->PushOverlay(std::move(overlay));
    rawOverlay->OnAttach();
    CH_CORE_INFO("Overlay Attached: {}", rawOverlay->GetName());
}

void Application::PushOverlay(Layer* overlay)
{
    PushOverlay(std::unique_ptr<Layer>(overlay));
}

void Application::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(CH_BIND_EVENT_FN(Application::OnWindowClose));
    dispatcher.Dispatch<WindowResizeEvent>(CH_BIND_EVENT_FN(Application::OnWindowResize));

    // Propagate events from top to bottom (overlays first)
    // We use a copy of the layer stack to avoid iterator invalidation if a layer is removed during event handling
    auto layers = m_LayerStack->GetLayerPointersSnapshot();
    for (auto it = layers.rbegin(); it != layers.rend(); ++it)
    {
        if (e.Handled)
        {
            break;
        }
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
    while (m_Running && !m_Window->ShouldClose())
    {
        CH_PROFILE_FUNCTION();

        ExecuteMainThreadQueue();

        // 1. Time Tracking
        float time = (float)glfwGetTime();

        // FPS Capping
        int targetFPS = m_Window->GetTargetFramesPerSecond();
        if (targetFPS > 0)
        {
            float minFrameTime = 1.0f / (float)targetFPS;
            while (time - m_LastFrameTime < minFrameTime)
            {
                time = (float)glfwGetTime();
            }
        }

        m_DeltaTime = Timestep(time - m_LastFrameTime);
        m_LastFrameTime = time;

        // 2. Input Polling
        Input::Update();

        // 3. Core Systems Update
        AssetManager::Get().Update();
        if (auto project = Project::GetActive())
        {
            Audio::Get().Update(m_DeltaTime);
        }

        // 4. Layers Update & Rendering
        Profiler::BeginFrame();
        {
            CH_PROFILE_SCOPE("MainThread_Frame");

            if (!m_Minimized)
            {
                // -- Logic/Simulation --

                // 1. Variable Update
                for (auto& layer : *m_LayerStack)
                {
                    if (layer->IsEnabled())
                    {
                        layer->OnUpdate(m_DeltaTime);
                    }
                }

                // 2. Fixed Update
                m_Accumulator += (float)m_DeltaTime;
                while (m_Accumulator >= m_FixedTimestep)
                {
                    for (auto& layer : *m_LayerStack)
                    {
                        if (layer->IsEnabled())
                        {
                            layer->OnFixedUpdate(Timestep(m_FixedTimestep));
                        }
                    }
                    m_Accumulator -= m_FixedTimestep;
                }

                // -- Rendering --
                m_Window->BeginFrame();

                for (auto& layer : *m_LayerStack)
                {
                    if (layer->IsEnabled())
                    {
                        layer->OnRender(m_DeltaTime);
                    }
                }

                // ImGui
                m_ImGuiLayer->Begin();
                for (auto& layer : *m_LayerStack)
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

std::filesystem::path Application::GetExecutableDirectory()
{
#if CH_PLATFORM_WINDOWS
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::filesystem::path(path).parent_path();
#elif CH_PLATFORM_LINUX
    char path[1024];
    ssize_t count = readlink("/proc/self/exe", path, sizeof(path));
    if (count != -1)
    {
        return std::filesystem::path(std::string(path, count)).parent_path();
    }
#endif
    return std::filesystem::current_path();
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
