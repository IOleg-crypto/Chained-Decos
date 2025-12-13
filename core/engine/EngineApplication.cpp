#include "EngineApplication.h"
#include "Engine.h"
#include "IApplication.h"
#include <cassert>
#include <raylib.h>
#include <stdexcept>

EngineApplication::EngineApplication(Config config, IApplication *application)
    : m_app(application), m_config(std::move(config))
{
    assert(m_app != nullptr && "Application instance cannot be null!");
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

void EngineApplication::Initialize()
{
    TraceLog(LOG_INFO, "[EngineApplication] Initializing application...");

    // Step 1: Configuration
    if (m_app)
    {
        IApplication::EngineConfig config;
        config.width = m_config.width;
        config.height = m_config.height;
        config.windowName = m_config.windowName;

        m_app->OnConfigure(config);

        // Apply back to m_config
        m_config.width = config.width;
        m_config.height = config.height;
        m_config.windowName = config.windowName;
    }

    // Step 2: Create managers (no dependencies)
    InitializeManagers();

    // Step 3: Create window (needs RenderManager)
    InitializeWindow();

    // Step 4: Create engine (needs managers)
    InitializeEngine();

    // Step 5: Initialize application (needs engine)
    InitializeApplication();

    TraceLog(LOG_INFO, "[EngineApplication] Application initialized successfully!");
}

void EngineApplication::InitializeManagers()
{
    TraceLog(LOG_INFO, "[EngineApplication] Creating managers...");

    m_renderManager = std::make_unique<RenderManager>();
    m_inputManager = std::make_unique<InputManager>();
    m_audioManager = std::make_unique<AudioManager>();
    m_guiContext = std::make_unique<gui::GUIContext>();

    TraceLog(LOG_INFO, "[EngineApplication] Managers created");
}

void EngineApplication::InitializeWindow()
{
    TraceLog(LOG_INFO, "[EngineApplication] Initializing window...");

    if (!m_renderManager->Initialize(m_config.width, m_config.height, m_config.windowName.c_str()))
    {
        TraceLog(LOG_FATAL, "[EngineApplication] Failed to initialize RenderManager (Window)");
        throw std::runtime_error("Failed to initialize RenderManager");
    }

    TraceLog(LOG_INFO, "[EngineApplication] Window initialized");
}

void EngineApplication::InitializeEngine()
{
    TraceLog(LOG_INFO, "[EngineApplication] Creating engine...");

    // Constructor injection - pass references to managers!
    m_engine = std::make_unique<Engine>(*m_renderManager, *m_inputManager, *m_audioManager);

    // Set GUI context pointer (Engine doesn't own it, just has access)
    if (m_guiContext)
    {
        m_engine->SetGUIContext(m_guiContext.get());
    }

    if (!m_engine->Initialize())
    {
        throw std::runtime_error("Failed to initialize Engine");
    }

    TraceLog(LOG_INFO, "[EngineApplication] Engine created");
}

void EngineApplication::InitializeApplication()
{
    TraceLog(LOG_INFO, "[EngineApplication] Initializing application...");

    if (m_app)
    {
        m_app->OnRegister(*m_engine);
        m_app->OnStart(*m_engine);
    }

    TraceLog(LOG_INFO, "[EngineApplication] Application initialized");
}

void EngineApplication::Update()
{
    float deltaTime = GetFrameTime();

    if (m_engine)
    {
        m_inputManager->Update(deltaTime);
        m_engine->Update(deltaTime);

        if (m_app)
        {
            m_app->OnUpdate(deltaTime, *m_engine);
        }
    }
}

void EngineApplication::Render()
{
    m_renderManager->BeginFrame();

    if (m_app)
    {
        m_app->OnRender(*m_engine);
    }

    m_renderManager->EndFrame();
}

void EngineApplication::Shutdown()
{
    TraceLog(LOG_INFO, "[EngineApplication] Shutting down...");

    if (m_app)
    {
        m_app->OnShutdown();
    }

    if (m_engine)
    {
        m_engine->Shutdown();
    }

    if (m_renderManager)
    {
        m_renderManager->Shutdown();
    }

    if (m_inputManager)
    {
        m_inputManager->Shutdown();
    }

    if (m_audioManager)
    {
        m_audioManager->Shutdown();
    }

    m_initialized = false;
    TraceLog(LOG_INFO, "[EngineApplication] Shutdown complete");
}
