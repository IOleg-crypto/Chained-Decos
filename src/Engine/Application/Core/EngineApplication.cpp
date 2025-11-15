//
// EngineApplication.cpp - Base application implementation
// Created by Auto on 2025
//
#include "EngineApplication.h"
#include "Engine/Render/Core/RenderManager.h"
#include "Engine/Input/Core/InputManager.h"
#include "Engine/Kernel/Core/KernelServices.h"
#include "Engine/Engine.h"
#include <raylib.h>

EngineApplication::EngineApplication(const Config& config)
    : m_config(config)
{
}

EngineApplication::~EngineApplication()
{
    if (m_initialized) {
        Shutdown();
    }
}

void EngineApplication::Run()
{
    if (!m_initialized) {
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
    OnPreInitialize();
    
    // Step 2: Create Kernel
    m_kernel = std::make_unique<Kernel>();
    ConfigureKernel(m_kernel.get());
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
    
    // Update window name if needed
    // (Engine doesn't have SetWindowName yet, can be added later)
    
    // Step 5: Configure ModuleManager
    if (auto moduleManager = m_engine->GetModuleManager()) {
        ConfigureModuleManager(moduleManager);
    }
    
    // Step 6: Allow project to initialize its services
    OnInitializeServices();
    
    // Step 7: Initialize Engine (registers Render and Input services)
    m_engine->Init();
    
    // Step 7.5: Register Engine as service
    m_kernel->RegisterService<EngineService>(
        std::make_shared<EngineService>(m_engine.get()));
    TraceLog(LOG_INFO, "[EngineApplication] EngineService registered");
    
    // Step 8: Allow project to register additional services after Engine
    OnRegisterEngineServices();
    
    // Step 9: Register project modules (REQUIRED)
    OnRegisterProjectModules();
    
    // Step 10: Register project services
    OnRegisterProjectServices();
    
    // Step 11: Before module initialization
    OnPreInitializeModules();
    
    // Step 12: Initialize all modules
    if (auto moduleManager = m_engine->GetModuleManager()) {
        moduleManager->InitializeAllModules();
    }
    
    // Step 13: After full initialization
    OnPostInitialize();
    
    TraceLog(LOG_INFO, "[EngineApplication] Application initialized successfully!");
}

void EngineApplication::Update()
{
    float deltaTime = GetFrameTime();
    
    OnPreUpdate(deltaTime);
    
    if (m_engine) {
        m_engine->Update();  // Updates Kernel and Modules
        m_engine->GetInputManager().ProcessInput();
    }
    
    OnPostUpdate(deltaTime);
}

void EngineApplication::Render()
{
    OnPreRender();
    
    if (m_engine) {
        // Begin frame
        m_engine->GetRenderManager()->BeginFrame();
        
        // Render modules (systems)
        if (auto moduleManager = m_engine->GetModuleManager()) {
            moduleManager->RenderAllModules();
        }
        
        // Allow project to render its own
        OnPostRender();
        
        // End frame
        m_engine->GetRenderManager()->EndFrame();
    }
    
    if (m_kernel) {
        m_kernel->Render();  // Renders services (after EndFrame, for ImGui etc.)
    }
}

void EngineApplication::Shutdown()
{
    TraceLog(LOG_INFO, "[EngineApplication] Shutting down application...");
    
    OnPreShutdown();
    
    // Shutdown in reverse order
    if (auto moduleManager = m_engine->GetModuleManager()) {
        moduleManager->ShutdownAllModules();
    }
    
    if (m_kernel) {
        m_kernel->Shutdown();
    }
    
    if (m_engine) {
        m_engine->Shutdown();
    }
    
    TraceLog(LOG_INFO, "[EngineApplication] Application shut down.");
}

