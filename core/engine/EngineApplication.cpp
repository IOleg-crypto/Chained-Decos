#include "EngineApplication.h"
#include "Engine.h"
#include "IApplication.h"

#include "components/input/Core/InputManager.h"
#include "components/rendering/Core/RenderManager.h"
#include <cassert>
#include <raylib.h>

EngineApplication::EngineApplication(Config config, IApplication *application)
    : m_app(application), m_config(std::move(config))
{
    assert(m_app != nullptr && "Application instance cannot be null!");

    // Create the Engine singleton
    m_engine = std::make_shared<Engine>();
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

    // Step 3: Initialize all modules
    if (auto moduleManager = m_engine->GetModuleManager())
    {
        moduleManager->InitializeAllModules(m_engine.get());
    }

    // Step 4: Start
    if (m_app)
    {
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
