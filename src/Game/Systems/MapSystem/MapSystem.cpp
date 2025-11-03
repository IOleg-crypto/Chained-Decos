#include "MapSystem.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "../../Managers/MapManager.h"
#include "Engine/World/World.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Render/RenderManager.h"
#include "../../Player/Player.h"
#include "../../Menu/Menu.h"
#include <raylib.h>

MapSystem::MapSystem()
    : m_mapManager(nullptr),
      m_kernel(nullptr),
      m_worldManager(nullptr),
      m_collisionManager(nullptr),
      m_modelLoader(nullptr),
      m_renderManager(nullptr),
      m_player(nullptr),
      m_menu(nullptr),
      m_engine(nullptr)
{
}

MapSystem::~MapSystem()
{
    Shutdown();
}

bool MapSystem::Initialize(Kernel* kernel)
{
    if (!kernel) {
        TraceLog(LOG_ERROR, "[MapSystem] Kernel is null");
        return false;
    }

    m_kernel = kernel;
    TraceLog(LOG_INFO, "[MapSystem] Initializing...");

    // Get engine dependencies through Kernel
    auto worldService = kernel->GetService<WorldService>(Kernel::ServiceType::World);
    auto collisionService = kernel->GetService<CollisionService>(Kernel::ServiceType::Collision);
    auto modelsService = kernel->GetService<ModelsService>(Kernel::ServiceType::Models);
    auto renderService = kernel->GetService<RenderService>(Kernel::ServiceType::Render);
    auto engineService = kernel->GetService<EngineService>(Kernel::ServiceType::Engine);
    
    // Player and Menu can be from other systems
    auto playerService = kernel->GetService<PlayerService>(Kernel::ServiceType::Player);
    auto menuService = kernel->GetService<MenuService>(Kernel::ServiceType::Menu);

    // Validate required engine dependencies
    if (!worldService || !collisionService || !modelsService || !renderService) {
        TraceLog(LOG_ERROR, "[MapSystem] Required engine services not found");
        return false;
    }

    m_worldManager = worldService->world;
    m_collisionManager = collisionService->cm;
    m_modelLoader = modelsService->models;
    m_renderManager = renderService->renderManager;
    m_engine = engineService ? engineService->engine : nullptr;
    
    // Player and Menu can be nullptr if their systems aren't initialized yet
    m_player = playerService ? playerService->player : nullptr;
    m_menu = menuService ? menuService->menu : nullptr;

    // Create our own components
    try {
        // MapManager requires: Player, CollisionManager, ModelLoader, RenderManager, Kernel, Menu (optional)
        m_mapManager = std::make_unique<MapManager>(
            m_player,           // Can be nullptr for now
            m_collisionManager,
            m_modelLoader,
            m_renderManager,
            kernel,
            m_menu              // Optional
        );
        TraceLog(LOG_INFO, "[MapSystem] MapManager created");

        // Register services in Initialize so they're available to other systems
        RegisterServices(kernel);

        TraceLog(LOG_INFO, "[MapSystem] Initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[MapSystem] Failed to create components: %s", e.what());
        return false;
    }
}

void MapSystem::Shutdown()
{
    TraceLog(LOG_INFO, "[MapSystem] Shutting down...");

    // Clean up our own resources (we own them)
    m_mapManager.reset();
    
    // Dependencies - references only, don't delete
    m_kernel = nullptr;
    m_worldManager = nullptr;
    m_collisionManager = nullptr;
    m_modelLoader = nullptr;
    m_renderManager = nullptr;
    m_player = nullptr;
    m_menu = nullptr;
    m_engine = nullptr;
    
    TraceLog(LOG_INFO, "[MapSystem] Shutdown complete");
}

void MapSystem::Update(float deltaTime)
{
    // Map update logic if needed
    (void)deltaTime;
}

void MapSystem::Render()
{
    // MapSystem::Render is called through ModuleManager::RenderAllModules()
    // RenderEditorMap() and RenderSpawnZone() are now called in RenderingSystem::RenderGameWorld()
    // for correct rendering order (inside BeginMode3D/EndMode3D)
    // Empty function - rendering is handled by RenderingSystem
}

void MapSystem::RegisterServices(Kernel* kernel)
{
    if (!kernel) {
        return;
    }

    TraceLog(LOG_INFO, "[MapSystem] Registering services...");

    // Register our own components as services
    if (m_mapManager) {
        kernel->RegisterService<MapManagerService>(
            Kernel::ServiceType::MapManager,
            std::make_shared<MapManagerService>(m_mapManager.get())
        );
        TraceLog(LOG_INFO, "[MapSystem] MapManagerService registered");
    }
}

std::vector<std::string> MapSystem::GetDependencies() const
{
    // Base system - no dependencies on other game systems
    // But depends on engine services which are always available
    return {};
}

