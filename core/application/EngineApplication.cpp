#include "core/application/EngineApplication.h"
#include "Engine.h"
#include "core/Log.h"
#include "core/application/IApplication.h"
#include "core/imgui/ImGuiLayer.h"
#include "core/window/Window.h"

#include "components/input/core/InputManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/interfaces/IGuiManager.h"
#include <cassert>
#include <raylib.h>

namespace CHEngine
{

EngineApplication::EngineApplication(Config config, IApplication *application)
    : m_app(application), m_config(std::move(config))
{
    assert(m_app != nullptr && "Application instance cannot be null!");

    // Create the Engine singleton
    // Create the Engine singleton
    m_engine = std::make_shared<CHEngine::Engine>();
    m_app->SetAppRunner(this);
}

EngineApplication::~EngineApplication()
{
    if (m_initialized)
    {
        Shutdown();
    }
}

void EngineApplication::Run()
{
    if (!m_initialized)
    {
        Initialize();
        m_initialized = true;
    }

    while (!m_engine->ShouldExit())
    {
        Update();
        Render();
    }

    Shutdown();
}

void EngineApplication::PushLayer(Layer *layer)
{
    m_LayerStack.PushLayer(layer);
}

void EngineApplication::PushOverlay(Layer *overlay)
{
    m_LayerStack.PushOverlay(overlay);
}

void EngineApplication::PopLayer(Layer *layer)
{
    m_LayerStack.PopLayer(layer);
}

void EngineApplication::PopOverlay(Layer *overlay)
{
    m_LayerStack.PopOverlay(overlay);
}

void EngineApplication::OnEvent(Event &e)
{
    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
    {
        if (e.Handled)
            break;
        (*it)->OnEvent(e);
    }

    if (!e.Handled && m_app)
    {
        m_app->OnEvent(e);
    }
}

void EngineApplication::Initialize()
{
    CD_CORE_INFO("Initializing application...");

    // Step 1: Configuration
    if (m_app)
    {
        IApplication::EngineConfig config;
        // Set defaults from constructor config
        config.width = m_config.width;
        config.height = m_config.height;
        config.windowName = m_config.windowName;

        m_app->OnConfigure(config);

        // Apply back to m_config
        m_config.width = config.width;
        m_config.height = config.height;
        m_config.windowName = config.windowName;
    }

    // Step 2: Register modules and services
    m_app->OnRegister();

    // Step 2.5: Initialize Engine & Window (Must be done before modules initialize)
    CHEngine::WindowProps props(m_config.windowName, m_config.width, m_config.height,
                                     m_config.fullscreen, m_config.vsync);
    if (!m_engine->Initialize(props))
    {
        CD_CORE_FATAL("[EngineApplication] Failed to initialize Engine!");
        throw std::runtime_error("Failed to initialize Engine");
    }

    // Step 3: Initialize all modules
    if (auto moduleManager = m_engine->GetModuleManager())
    {
        moduleManager->InitializeAllModules(m_engine.get());
    }

    // Step 4: Start
    if (m_app)
    {
        // Connect Input Events
        if (m_engine->GetInputManager())
        {
            m_engine->GetInputManager()->SetEventCallback([this](Event &e) { this->OnEvent(e); });
        }

        // Push ImGuiLayer as overlay
        PushOverlay(new ImGuiLayer());

        m_app->OnStart();
    }

    CD_CORE_INFO("Application initialized successfully!");
}

void EngineApplication::Update()
{
    float deltaTime = GetFrameTime();

    if (m_engine)
    {
        m_engine->Update(deltaTime); // Updates Kernel and Modules
        m_engine->GetInputManager()->ProcessInput();
    }

    // Update Layers (Bottom -> Top)
    for (Layer *layer : m_LayerStack)
        layer->OnUpdate(deltaTime);

    if (m_app)
        m_app->OnUpdate(deltaTime);
}

void EngineApplication::Render()
{
    if (m_engine)
    {
        // Begin frame
        m_engine->GetRenderManager()->BeginFrame();

        // Render modules (systems)
        if (auto moduleManager = m_engine->GetModuleManager())
        {
            moduleManager->RenderAllModules();
        }

        // Render Layers (Bottom -> Top)
        for (Layer *layer : m_LayerStack)
            layer->OnRender();

        // Allow project to render its own
        if (m_app)
            m_app->OnRender();

        // Begin ImGui frame
        ImGuiLayer *imguiLayer = nullptr;
        for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        {
            if ((*it)->GetName() == "ImGuiLayer")
            {
                imguiLayer = static_cast<ImGuiLayer *>(*it);
                break;
            }
        }

        if (imguiLayer)
        {
            imguiLayer->Begin();

            if (m_app)
                m_app->OnImGuiRender();

            // Render Custom GUI (Above the game, but maybe below ImGUI dev tools)
            if (auto gui = m_engine->GetGuiManager())
            {
                gui->Render();
            }

            imguiLayer->End();
        }

        // End frame
        m_engine->GetRenderManager()->EndFrame();
    }
}

void EngineApplication::Shutdown()
{
    CD_CORE_INFO("Shutting down application...");

    if (m_app)
        m_app->OnShutdown();

    // Shutdown in reverse order
    if (auto moduleManager = m_engine->GetModuleManager())
    {
        moduleManager->ShutdownAllModules();
    }

    if (m_engine)
    {
        m_engine->Shutdown();
    }

    CD_CORE_INFO("Application shut down.");
}

CHEngine::Engine *EngineApplication::GetEngine() const
{
    return m_engine.get();
}
EngineApplication::Config &EngineApplication::GetConfig()
{
    return m_config;
}
const EngineApplication::Config &EngineApplication::GetConfig() const
{
    return m_config;
}
} // namespace CHEngine
