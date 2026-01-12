#include "application.h"
#include "engine/audio/audio_manager.h"
#include "engine/core/input.h"
#include "engine/core/log.h"
#include "engine/core/thread_dispatcher.h"
#include "engine/physics/physics.h"
#include "engine/renderer/asset_manager.h"
#include "engine/renderer/render.h"
#include "engine/renderer/scene_render.h"
#include <raylib.h>
#include <rlImGui.h>

namespace CHEngine
{
Application *Application::s_Instance = nullptr;

Application::Application(const Config &config)
{
    CH_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

    // Log::Init(); // Removed
    CH_CORE_INFO("Initializing Engine..."); // Added
    SetTraceLogLevel(LOG_WARNING);          // Reduce spam
    InitWindow(config.Width, config.Height, config.Title.c_str());
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); // Prevent ESC from closing the app
    rlImGuiSetup(true);   // Added
    m_Running = true;

    Render::Init();
    SceneRender::Init();
    ThreadDispatcher::Init();
    Physics::Init();
    AssetManager::Init();
    AudioManager::Init(); // Added

    CH_CORE_INFO("Application Initialized: %s (%dx%d)", config.Title, config.Width, config.Height);
}

Application::~Application()
{
    AssetManager::Shutdown();
    ThreadDispatcher::Shutdown();
    Physics::Shutdown();
    SceneRender::Shutdown();
    Render::Shutdown();
    CloseWindow();
    s_Instance = nullptr;
}

void Application::Init(const Config &config)
{
    // For static access if needed, though constructor handles it
}

void Application::Shutdown()
{
    CH_CORE_INFO("Shutting down Engine...");

    AudioManager::Shutdown();
    rlImGuiShutdown();
    CloseWindow();

    CH_CORE_INFO("Engine Shutdown Successfully.");
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
    // Clear per-frame input state
    Input::UpdateState();

    ThreadDispatcher::ExecuteMainThreadQueue();

    PollEvents();

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

void Application::PollEvents()
{
    // 1. Keyboard Events
    int key = GetKeyPressed();
    while (key != 0)
    {
        Input::OnKeyPressed(key);
        KeyPressedEvent e(key, false);
        OnEvent(e);
        key = GetKeyPressed();
    }

    // Track key releases
    static bool s_KeysDown[512] = {false};
    for (int i = 32; i < 349; i++)
    {
        if (IsKeyDown(i))
        {
            if (!s_KeysDown[i])
                s_KeysDown[i] = true;
        }
        else
        {
            if (s_KeysDown[i])
            {
                Input::OnKeyReleased(i);
                KeyReleasedEvent e(i);
                OnEvent(e);
                s_KeysDown[i] = false;
            }
        }
    }

    // 2. Mouse Events
    for (int i = 0; i < 7; i++)
    {
        if (IsMouseButtonPressed(i))
        {
            Input::OnMouseButtonPressed(i);
            MouseButtonPressedEvent e(i);
            OnEvent(e);
        }
        if (IsMouseButtonReleased(i))
        {
            Input::OnMouseButtonReleased(i);
            MouseButtonReleasedEvent e(i);
            OnEvent(e);
        }
    }

    static Vector2 lastMousePos = {-1, -1};
    Vector2 currentMousePos = GetMousePosition();
    if (currentMousePos.x != lastMousePos.x || currentMousePos.y != lastMousePos.y)
    {
        MouseMovedEvent e(currentMousePos.x, currentMousePos.y);
        OnEvent(e);
        lastMousePos = currentMousePos;
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0)
    {
        MouseScrolledEvent e(0, wheel);
        OnEvent(e);
    }
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
} // namespace CHEngine
