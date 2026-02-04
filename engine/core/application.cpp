#include "application.h"
#include "engine/core/assert.h"
#include "engine/scene/scene_events.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/physics/physics.h"
#include "engine/graphics/render.h"
#include "engine/graphics/asset_manager.h"
#include "engine/graphics/environment.h"
#include "engine/graphics/font_asset.h"
#include "engine/scene/project.h"
#include "engine/scene/component_serializer.h"
#include "engine/core/imgui_layer.h"
#include "engine/core/window.h"
#include "engine/core/thread_pool.h"
#include "engine/core/layer.h"
#include "rlgl.h"

#include <ranges>
#include <filesystem>
#include <algorithm>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace CHEngine
{
    Application *Application::s_Instance = nullptr;

    Application::Application(const ApplicationSpecification& specification)
        : m_Specification(specification)
    {
        CH_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;

        if (!m_Specification.WorkingDirectory.empty())
            std::filesystem::current_path(m_Specification.WorkingDirectory);

        CH_CORE_INFO("Initializing Engine Core...");

        // --- Window Setup ---
        WindowProps windowProps;
        windowProps.Title = m_Specification.Name;
        
        // ImGui Ini path setup
        std::string iniName = m_Specification.Name;
        std::replace(iniName.begin(), iniName.end(), ' ', '_');
        std::transform(iniName.begin(), iniName.end(), iniName.begin(), ::tolower);
        
        #ifdef PROJECT_ROOT_DIR
        windowProps.IniFilename = std::string(PROJECT_ROOT_DIR) + "/imgui_" + iniName + ".ini";
        #else
        windowProps.IniFilename = "imgui_" + iniName + ".ini";
        #endif

        if (Project::GetActive())
        {
            auto &projConfig = Project::GetActive()->GetConfig();
            windowProps.Width = projConfig.Window.Width;
            windowProps.Height = projConfig.Window.Height;
            windowProps.VSync = projConfig.Window.VSync;
            windowProps.Resizable = projConfig.Window.Resizable;
        }

        // --- System Initialization ---
        m_ThreadPool = std::make_unique<ThreadPool>();
        m_Window = std::make_unique<Window>(windowProps);
        m_Running = true;

        ComponentSerializer::Init();
        Render::Init();
        Physics::Init();

        // Audio setup
        InitAudioDevice();
        if (IsAudioDeviceReady())
            CH_CORE_INFO("Audio Device Initialized Successfully");
        else
            CH_CORE_ERROR("Failed to initialize Audio Device!");
        
        // ImGui Layer setup
        m_ImGuiLayer = new ImGuiLayer();
        PushOverlay(m_ImGuiLayer);

        CH_CORE_INFO("Application Initialized: {}", m_Specification.Name);
    }

    Application::~Application()
    {
        if (m_Running)
        {
            CH_CORE_INFO("Shutting down Application...");
            CloseAudioDevice();
            m_LayerStack.Shutdown();
            m_Window.reset();
        }
        
        s_Instance = nullptr;
        CH_CORE_INFO("Engine Shutdown Successfully.");
    }

    void Application::PushLayer(Layer *layer)
    {
        CH_CORE_ASSERT(layer, "Layer is null!");
        m_LayerStack.PushLayer(layer);
        layer->OnAttach();
        CH_CORE_INFO("Layer Attached: {}", layer->GetName());
    }

    void Application::PushOverlay(Layer *overlay)
    {
        CH_CORE_ASSERT(overlay, "Overlay is null!");
        m_LayerStack.PushOverlay(overlay);
        overlay->OnAttach();
        CH_CORE_INFO("Overlay Attached: {}", overlay->GetName());
    }

    void Application::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(CH_BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(CH_BIND_EVENT_FN(Application::OnWindowResize));

        // Propagate events from top to bottom (overlays first)
        for (const auto & it : std::ranges::reverse_view(m_LayerStack))
        {
            if (e.Handled) break;
            if (it->IsEnabled()) it->OnEvent(e);
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
        Render::SetViewport(0, 0, e.GetWidth(), e.GetHeight());

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
            func();
    }

    void Application::Run()
    {
        ::SetExitKey(NULL);

        while (m_Running && !m_Window->ShouldClose())
        {
            CH_PROFILE_FUNCTION();

            ExecuteMainThreadQueue();

            // 1. Time Tracking
            float time = (float)GetTime();
            m_DeltaTime = Timestep(time - m_LastFrameTime);
            m_LastFrameTime = time;

            // 2. Input Polling
            Input::PollEvents();

            // 3. Core Systems Update
            if (auto project = Project::GetActive())
                if (auto assetManager = project->GetAssetManager())
                    assetManager->Update();

            // 4. Layers Update & Rendering
            Profiler::BeginFrame();
            {
                CH_PROFILE_SCOPE("MainThread_Frame");

                if (!m_Minimized)
                {
                    // Logic/Simulation
                    for (auto layer : m_LayerStack)
                        if (layer->IsEnabled()) layer->OnUpdate(m_DeltaTime);

                    // Rendering
                    m_Window->BeginFrame();
                    
                    for (auto layer : m_LayerStack)
                        if (layer->IsEnabled()) layer->OnRender(m_DeltaTime);

                    // ImGui
                    m_ImGuiLayer->Begin();
                    for (auto layer : m_LayerStack)
                        if (layer->IsEnabled()) layer->OnImGuiRender();
                    m_ImGuiLayer->End();

                    m_Window->EndFrame();
                }
            }
            Profiler::EndFrame();
        }
    }

} // namespace CHEngine
