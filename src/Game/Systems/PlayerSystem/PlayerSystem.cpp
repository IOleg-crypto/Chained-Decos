#include "PlayerSystem.h"
#include "Engine/Kernel/Core/Kernel.h"
#include "Engine/Kernel/Core/KernelServices.h"
#include "Engine/Engine.h"
#include "../../Player/Player.h"
#include "../../Managers/PlayerManager.h"
#include "../../Menu/Menu.h"
#include "../../Menu/ConsoleManagerHelpers.h"
#include "Engine/Collision/Manager/CollisionManager.h"
#include "Engine/Model/Core/Model.h"
#include <raylib.h>

PlayerSystem::PlayerSystem()
    : m_player(nullptr), m_playerManager(nullptr),
      m_kernel(nullptr),
      m_collisionManager(nullptr), m_mapManager(nullptr),
      m_models(nullptr), m_engine(nullptr)
{
}

PlayerSystem::~PlayerSystem()
{
    Shutdown();
}

bool PlayerSystem::Initialize(Kernel* kernel)
{
    if (!kernel) {
        TraceLog(LOG_ERROR, "[PlayerSystem] Kernel is null");
        return false;
    }

    m_kernel = kernel;
    TraceLog(LOG_INFO, "[PlayerSystem] Initializing...");

    // Get engine dependencies through Kernel
    auto collisionService = kernel->GetService<CollisionService>(Kernel::ServiceType::Collision);
    auto modelsService = kernel->GetService<ModelsService>(Kernel::ServiceType::Models);
    auto mapService = kernel->GetService<MapManagerService>(Kernel::ServiceType::MapManager);
    auto engineService = kernel->GetService<EngineService>(Kernel::ServiceType::Engine);

    // Validate required engine dependencies
    if (!collisionService || !modelsService) {
        TraceLog(LOG_ERROR, "[PlayerSystem] Required engine services not found");
        return false;
    }

    m_collisionManager = collisionService->cm;
    m_models = modelsService->models;
    
    // MapManager can be nullptr if MapSystem isn't initialized yet
    m_mapManager = mapService ? mapService->mapManager : nullptr;

    // Get Engine through EngineService
    m_engine = engineService ? engineService->engine : nullptr;
    
    if (!m_engine) {
        TraceLog(LOG_WARNING, "[PlayerSystem] Engine service not found - some features may be limited");
    }

    // Create our own components
    try {
        m_player = std::make_unique<Player>();
        TraceLog(LOG_INFO, "[PlayerSystem] Player created");

        // Create PlayerManager with dependencies
        // PlayerManager requires: Player, CollisionManager, ModelLoader, Engine, MapManager
        // Engine and MapManager can be nullptr for now if systems aren't initialized yet
        if (m_collisionManager && m_models) {
            m_playerManager = std::make_unique<PlayerManager>(
                m_player.get(),
                m_collisionManager,
                m_models,
                m_engine,  // Can be nullptr
                m_mapManager  // Can be nullptr, updated later
            );
            TraceLog(LOG_INFO, "[PlayerSystem] PlayerManager created");
        } else {
            TraceLog(LOG_WARNING, "[PlayerSystem] Cannot create PlayerManager - missing dependencies");
        }

        // Register services in Initialize so they're available to other systems
        RegisterServices(kernel);

        TraceLog(LOG_INFO, "[PlayerSystem] Initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "[PlayerSystem] Failed to create components: %s", e.what());
        return false;
    }
}

void PlayerSystem::Shutdown()
{
    TraceLog(LOG_INFO, "[PlayerSystem] Shutting down...");

    // Clean up our own resources (we own them)
    m_playerManager.reset();
    m_player.reset();
    
    // Dependencies - references only, don't delete
    m_kernel = nullptr;
    m_collisionManager = nullptr;
    m_mapManager = nullptr;
    m_models = nullptr;
    m_engine = nullptr;
    
    TraceLog(LOG_INFO, "[PlayerSystem] Shutdown complete");
}

void PlayerSystem::Update(float deltaTime)
{
    // MapManager should be available since PlayerSystem depends on MapSystem
    // But check anyway
    if (!m_mapManager && m_kernel) {
        auto mapService = m_kernel->GetService<MapManagerService>(Kernel::ServiceType::MapManager);
        if (mapService && mapService->mapManager) {
            m_mapManager = mapService->mapManager;
            TraceLog(LOG_INFO, "[PlayerSystem] MapManager obtained from Kernel");
        }
    }
    
    if (!m_playerManager || !m_player) {
        return;
    }

    // Check if player is at uninitialized position
    // Player starts at (-999999, -999999, -999999) until InitPlayer() is called
    Vector3 pos = m_player->GetPlayerPosition();
    constexpr float UNINITIALIZED_THRESHOLD = -999000.0f;
    
    if (pos.y <= UNINITIALIZED_THRESHOLD || 
        (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f)) {
        // Player is not initialized yet - don't update
        return;
    }

    // Player has valid position - safe to update
    m_playerManager->UpdatePlayerLogic();
}

void PlayerSystem::Render()
{
    // Player rendering handled by RenderingSystem::RenderGameWorld()
    // This system focuses on logic only, not rendering
}

void PlayerSystem::RegisterServices(Kernel* kernel)
{
    if (!kernel) {
        return;
    }

    TraceLog(LOG_INFO, "[PlayerSystem] Registering services...");

    // Register our own components as services
    if (m_player) {
        kernel->RegisterService<PlayerService>(
            Kernel::ServiceType::Player,
            std::make_shared<PlayerService>(m_player.get())
        );
        TraceLog(LOG_INFO, "[PlayerSystem] PlayerService registered");
        
        // Dependency Injection: inject PlayerProvider into ConsoleManager
        UpdateConsoleManagerProviders(kernel);
        
        // Dependency Injection: inject camera into Menu
        auto menuService = kernel->GetService<MenuService>(Kernel::ServiceType::Menu);
        if (menuService && menuService->menu) {
            auto cameraController = m_player->GetCameraController();
            if (cameraController) {
                menuService->menu->SetCameraController(cameraController.get());
                TraceLog(LOG_INFO, "[PlayerSystem] CameraController injected into Menu");
            }
        }
    }

    if (m_playerManager) {
        kernel->RegisterService<PlayerManagerService>(
            Kernel::ServiceType::PlayerManager,
            std::make_shared<PlayerManagerService>(m_playerManager.get())
        );
        TraceLog(LOG_INFO, "[PlayerSystem] PlayerManagerService registered");
    }
}

std::vector<std::string> PlayerSystem::GetDependencies() const
{
    // Depends on MapSystem (for MapManager)
    // Note: "Map" is the module name of MapSystem (GetModuleName returns "Map")
    return {"Map"};
}

