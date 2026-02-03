#include "application.h"
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

#include <ranges>
#include <filesystem>
#include <algorithm>

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include "raylib.h"
#include "rlgl.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace CHEngine
{
    Application *Application::s_Instance = nullptr;

    Application::Application(const Config &config) : m_Config(config)
    {
        CH_CORE_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;
    }

    Application::~Application()
    {
        Shutdown();
        s_Instance = nullptr;
    }

    bool Application::Initialize(const Config &config)
    {
        CH_CORE_INFO("Initializing Engine Core...");

        WindowProps windowProps = config;

        // --- ImGui Configuration ---
        // Setup persistent ImGui ini file path based on project title
        std::string iniName = config.Title;
        std::replace(iniName.begin(), iniName.end(), ' ', '_');
        std::transform(iniName.begin(), iniName.end(), iniName.begin(), ::tolower);
        
        #ifdef PROJECT_ROOT_DIR
        windowProps.IniFilename = std::string(PROJECT_ROOT_DIR) + "/imgui_" + iniName + ".ini";
        #else
        windowProps.IniFilename = "imgui_" + iniName + ".ini";
        #endif

        // Ensure the target directory for the config exists
        std::filesystem::path iniPath(windowProps.IniFilename);
        if (iniPath.has_parent_path() && !std::filesystem::exists(iniPath.parent_path()))
        {
            std::filesystem::create_directories(iniPath.parent_path());
        }

        // Apply project-specific window overrides if an active project exists
        if (Project::GetActive())
        {
            auto &projConfig = Project::GetActive()->GetConfig();
            windowProps.Width = projConfig.Window.Width;
            windowProps.Height = projConfig.Window.Height;
            windowProps.VSync = projConfig.Window.VSync;
            windowProps.Resizable = projConfig.Window.Resizable;
        }

        // --- System Initialization ---
        m_Window = std::make_unique<Window>(windowProps);
        m_Running = true;

        if (config.WindowIcon.data != nullptr)
        {
            m_Window->SetWindowIcon(config.WindowIcon);
        }

        ComponentSerializer::Init();
        Render::Init();
        Physics::Init();

        // Audio setup
        InitAudioDevice();
        if (IsAudioDeviceReady())
            CH_CORE_INFO("Audio Device Initialized Successfully");
        else
            CH_CORE_ERROR("Failed to initialize Audio Device!");

        CH_CORE_INFO("Application Initialized: {}", config.Title);

        // Client-side initialization hook
        PostInitialize();
        
        return true;
    }

    void Application::Shutdown()
    {
        if (!m_Running) return;

        CH_CORE_INFO("Shutting down Engine Core...");

        CloseAudioDevice();
        m_LayerStack.Shutdown();
        
        Physics::Shutdown();
        Render::Shutdown();
        
        m_Window.reset();
        m_Running = false;
        
        CH_CORE_INFO("Engine Shutdown Successfully.");
    }

    void Application::PushLayer(Layer *layer)
    {
        CH_CORE_ASSERT(layer, "Layer is null!");
        s_Instance->m_LayerStack.PushLayer(layer);
        layer->OnAttach();
        CH_CORE_INFO("Layer Attached: {}", layer->GetName());
    }

    void Application::PushOverlay(Layer *overlay)
    {
        CH_CORE_ASSERT(overlay, "Overlay is null!");
        s_Instance->m_LayerStack.PushOverlay(overlay);
        overlay->OnAttach();
        CH_CORE_INFO("Overlay Attached: {}", overlay->GetName());
    }

    void Application::BeginFrame()
    {
        CH_PROFILE_FUNCTION();

        s_Instance->m_DeltaTime = GetFrameTime();
        s_Instance->m_Window->BeginFrame();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Application::EndFrame()
    {
        // Internal Raylib batch flush
        rlDrawRenderBatchActive();

        // Finalize ImGui and render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Multi-viewport support
        ImGuiIO &io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow *backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        s_Instance->m_Window->EndFrame();
    }

    bool Application::ShouldClose()
    {
        return !s_Instance->m_Running || s_Instance->m_Window->ShouldClose();
    }

    void Application::OnEvent(Event &e)
    {
        // Propagate events from top to bottom (overlays first)
        for (const auto & it : std::ranges::reverse_view(s_Instance->m_LayerStack))
        {
            if (e.Handled) break;
            if (it->IsEnabled()) it->OnEvent(e);
        }
    }

    void Application::Close()
    {
        m_Running = false;
    }

    void Application::Run()
    {
        while (m_Running && !m_Window->ShouldClose())
        {
            ProcessEvents();
            Simulate();
            Render();
        }
    }

    void Application::ProcessEvents()
    {
        // --- Library-First Input Handling ---
        // Instead of polling 512 keys, we leverage Window/GLFW/Raylib event handling.
        
        // Raylib handles the underlying event queue, we just wrap them into our event system
        // for higher-level consumption by layers.
        
        // 1. Keyboard Events (using Raylib's built-in state changes)
        int key;
        while ((key = GetKeyPressed()) != 0)
        {
            KeyPressedEvent e(key, false);
            OnEvent(e);
        }

        // For releases, we use a more efficient approach in the future (e.g. only checking keys known to be down),
        // but for Phase 6 we are minimizing the 'brute force' 512-poll.
        // For now, removing the heavy 512-poll completely.
        // If a layer needs 'KeyReleased', it should be handled via a more specific mechanism.

        // 2. Mouse Events
        auto handleMouse = [&](int button)
        {
            if (::IsMouseButtonPressed(button))
            {
                MouseButtonPressedEvent e(button);
                OnEvent(e);
            }
            if (::IsMouseButtonReleased(button))
            {
                MouseButtonReleasedEvent e(button);
                OnEvent(e);
            }
        };
        handleMouse(MOUSE_BUTTON_LEFT);
        handleMouse(MOUSE_BUTTON_RIGHT);
        handleMouse(MOUSE_BUTTON_MIDDLE);

        Vector2 currentMousePos = ::GetMousePosition();
        static Vector2 lastMousePos = {0, 0};
        if (currentMousePos.x != lastMousePos.x || currentMousePos.y != lastMousePos.y)
        {
            MouseMovedEvent e(currentMousePos.x, currentMousePos.y);
            OnEvent(e);
            lastMousePos = currentMousePos;
        }

        float wheel = ::GetMouseWheelMove();
        if (wheel != 0)
        {
            MouseScrolledEvent e(0, wheel);
            OnEvent(e);
        }

        // Time tracking
        float time = (float)GetTime();
        m_DeltaTime = Timestep(time - m_LastFrameTime);
        m_LastFrameTime = time;
    }

    void Application::Simulate()
    {
        AssetManager::Update();
        Profiler::BeginFrame();
        {
            CH_PROFILE_SCOPE("MainThread_Frame");
            if (!m_Minimized)
            {
                // Delegate update logic to layers
                for (auto layer : m_LayerStack)
                {
                    if (layer->IsEnabled()) layer->OnUpdate(m_DeltaTime);
                }
            }
        }
    }

    void Application::Render()
    {
        if (m_Minimized) return;

        BeginFrame();

        // Layer rendering
        for (auto layer : m_LayerStack)
        {
            if (layer->IsEnabled()) layer->OnRender(m_DeltaTime);
        }

        // ImGui rendering
        for (auto layer : m_LayerStack)
        {
            if (layer->IsEnabled()) layer->OnImGuiRender();
        }

        EndFrame();
        Profiler::EndFrame();
    }

    void Application::SetWindowIcon(const Image &icon) const 
    {
        if (m_Window) m_Window->SetWindowIcon(icon);
    }

} // namespace CHEngine
