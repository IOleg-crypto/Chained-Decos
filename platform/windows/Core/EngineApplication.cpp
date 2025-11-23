#include "core/object/kernel/Core/Kernel.h"
#include "Engine/Engine.h"
#include "servers/input/Core/InputManager.h"
#include "servers/rendering/Core/RenderManager.h"
#include <cassert>
#include <raylib.h>

EngineApplication::EngineApplication(IApplication *app, const Config &config)
    : m_app(app), m_config(config)
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

    while (!m_engine->ShouldClose())
    {
        Update();
        Render();
    }

    Shutdown();
}

void EngineApplication::Initialize()
{
    TraceLog(LOG_INFO, "[EngineApplication] Initializing application...");

    // Step 1: Configuration before initialization
    if (m_app)
        m_app->OnPreInitialize();

    // Step 2: Create Kernel
    m_kernel = std::make_unique<Kernel>();
    if (m_app)
        m_app->ConfigureKernel(m_kernel.get());
    m_kernel->Initialize();

    // Step 3: Create core Engine services
    auto renderManager = std::make_shared<RenderManager>();
    auto inputManager = std::make_shared<InputManager>();

    // Step 4: Create Engine with simplified config
    EngineConfig engineConfig;
    engineConfig.screenWidth = m_config.width;
    engineConfig.screenHeight = m_config.height;
    engineConfig.renderManager = renderManager;
    engineConfig.inputManager = inputManager;
    engineConfig.kernel = m_kernel.get();

    m_engine = std::make_unique<Engine>(engineConfig);

    // Pass Engine and Kernel to Application
    if (m_app)
    {
        m_app->SetKernel(m_kernel.get());
        m_app->SetEngine(m_engine.get());
    }

    // Update window name if needed
    // (Engine doesn't have SetWindowName yet, can be added later)

    // Step 5: Configure ModuleManager
    if (auto moduleManager = m_engine->GetModuleManager())
    {
        if (m_app)
            m_app->ConfigureModuleManager(moduleManager);
    }

    // Step 6: Allow project to initialize its services
    if (m_app)
        m_app->OnInitializeServices();

    // Step 7: Initialize Engine (registers Render and Input services)
    m_engine->Init();

    // Step 7.5: Register Engine as service
    m_kernel->RegisterService<EngineService>(std::make_shared<EngineService>(m_engine.get()));
    TraceLog(LOG_INFO, "[EngineApplication] EngineService registered");

    // Step 8: Allow project to register additional services after Engine
    if (m_app)
        m_app->OnRegisterEngineServices();

    // Step 9: Register project modules (REQUIRED)
    if (m_app)
        m_app->OnRegisterProjectModules();

    // Step 10: Register project services
    if (m_app)
        m_app->OnRegisterProjectServices();

    // Step 11: Before module initialization
    if (m_app)
        m_app->OnPreInitializeModules();

    // Step 12: Initialize all modules
    if (auto moduleManager = m_engine->GetModuleManager())
    {
        moduleManager->InitializeAllModules();
    }

    // Step 13: After full initialization
    if (m_app)
        m_app->OnPostInitialize();

    TraceLog(LOG_INFO, "[EngineApplication] Application initialized successfully!");
}

void EngineApplication::Update()
{
    float deltaTime = GetFrameTime();

    if (m_app)
        m_app->OnPreUpdate(deltaTime);

    if (m_engine)
    {
        m_engine->Update(); // Updates Kernel and Modules
        m_engine->GetInputManager().ProcessInput();
    }

    if (m_app)
        m_app->OnPostUpdate(deltaTime);
}

void EngineApplication::Render()
{
    if (m_app)
        m_app->OnPreRender();

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
            m_app->OnPostRender();

        // End frame
        m_engine->GetRenderManager()->EndFrame();
    }

    if (m_kernel)
    {
        m_kernel->Render(); // Renders services (after EndFrame, for ImGui etc.)
    }
}

void EngineApplication::Shutdown()
{
    TraceLog(LOG_INFO, "[EngineApplication] Shutting down application...");

    if (m_app)
        m_app->OnPreShutdown();

    // Shutdown in reverse order
    if (auto moduleManager = m_engine->GetModuleManager())
    {
        moduleManager->ShutdownAllModules();
    }

    if (m_kernel)
    {
        m_kernel->Shutdown();
    }

    if (m_engine)
    {
        m_engine->Shutdown();
    }

    TraceLog(LOG_INFO, "[EngineApplication] Application shut down.");
}
