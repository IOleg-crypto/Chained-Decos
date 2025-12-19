#include "core/application/EngineApplication.h"
#include "Engine.h"
#include "core/application/IApplication.h"

#include "components/input/Core/InputManager.h"
#include "components/rendering/Core/RenderManager.h"
#include <cassert>
#include <raylib.h>

namespace ChainedDecos
{

EngineApplication::EngineApplication(Config config, IApplication *application)
    : m_app(application), m_config(std::move(config))
{
    assert(m_app != nullptr && "Application instance cannot be null!");

    // Create the Engine singleton
    m_engine = std::make_shared<Engine>();
    m_app->SetAppRunner(this);
    if (!m_engine->Initialize())
    {
        throw std::runtime_error("Failed to initialize Engine!");
    }
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

void EngineApplication::OnEvent(Event &e)
{
    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
    {
        if (e.Handled)
            break;
        (*it)->OnEvent(e);
    }
}

void EngineApplication::Initialize()
{
    TraceLog(LOG_INFO, "[EngineApplication] Initializing application...");

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

    // Step 2.5: Initialize Window (Must be done before modules initialize)
    if (!m_engine->GetRenderManager()->Initialize(m_config.width, m_config.height,
                                                  m_config.windowName.c_str()))
    {
        TraceLog(LOG_FATAL, "[EngineApplication] Failed to initialize RenderManager (Window)");
        throw std::runtime_error("Failed to initialize RenderManager");
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
        m_app->OnStart();
    }

    TraceLog(LOG_INFO, "[EngineApplication] Application initialized successfully!");
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

        // End frame
        m_engine->GetRenderManager()->EndFrame();
    }
}

void EngineApplication::Shutdown()
{
    TraceLog(LOG_INFO, "[EngineApplication] Shutting down application...");

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

    TraceLog(LOG_INFO, "[EngineApplication] Application shut down.");
}

Engine *EngineApplication::GetEngine() const
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
} // namespace ChainedDecos




