#include "UIController.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "Engine/Engine.h"
#include "../../Menu/Menu.h"
#include "../../Menu/ConsoleManager.h"
#include <raylib.h>

UIController::UIController()
    : m_menu(nullptr), m_kernel(nullptr), m_engine(nullptr)
{
}

UIController::~UIController()
{
    Shutdown();
}

bool UIController::Initialize(Kernel* kernel)
{
    if (!kernel) {
        TraceLog(LOG_ERROR, "[UIController] Kernel is null");
        return false;
    }

    m_kernel = kernel;
    TraceLog(LOG_INFO, "[UIController] Initializing...");

    // Get engine dependencies through Kernel
    auto engineService = kernel->GetService<EngineService>(Kernel::ServiceType::Engine);

    // Engine not required for Menu, but preferred
    m_engine = engineService ? engineService->engine : nullptr;
    
    if (!m_engine) {
        TraceLog(LOG_WARNING, "[UIController] Engine service not found - Menu may have limited functionality");
    }

    // Create our own components
    try {
        m_menu = std::make_unique<Menu>();
        
        // Initialize Menu if Engine is available
        if (m_engine) {
            m_menu->Initialize(m_engine);
            m_menu->SetKernel(kernel);
            TraceLog(LOG_INFO, "[UIController] Menu initialized");
        } else {
            TraceLog(LOG_WARNING, "[UIController] Menu created but not fully initialized (no Engine)");
        }

        // Register services in Initialize so they're available to other systems
        RegisterServices(kernel);

        TraceLog(LOG_INFO, "[UIController] Initialized successfully");
        return true;
    }           
    catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[UIController] Failed to create components: %s", e.what());
        return false;
    }
}

void UIController::Shutdown()
{
    TraceLog(LOG_INFO, "[UIController] Shutting down...");

    // Clean up our own resources (we own them)
    if (m_menu) {
        m_menu.reset();
    }
    
    // Dependencies - references only, don't delete
    m_kernel = nullptr;
    m_engine = nullptr;
    
    TraceLog(LOG_INFO, "[UIController] Shutdown complete");
}

void UIController::Update(float deltaTime)
{
    if (m_menu) {
        m_menu->Update();
    }
    (void)deltaTime;
}

void UIController::Render()
{
    // Menu rendering handled separately by RenderManager
    // This system focuses on logic only
}

void UIController::RegisterServices(Kernel* kernel)
{
    if (!kernel) {
        return;
    }

    TraceLog(LOG_INFO, "[UIController] Registering services...");

    // Register our own components as services
    if (m_menu) {
        kernel->RegisterService<MenuService>(
            Kernel::ServiceType::Menu,
            std::make_shared<MenuService>(m_menu.get())
        );
        TraceLog(LOG_INFO, "[UIController] MenuService registered");
    }
}

std::vector<std::string> UIController::GetDependencies() const
{
    // No dependencies on other game systems
    return {};
}

ConsoleManager* UIController::GetConsoleManager() const
{
    if (!m_menu) {
        return nullptr;
    }
    return m_menu->GetConsoleManager();
}

