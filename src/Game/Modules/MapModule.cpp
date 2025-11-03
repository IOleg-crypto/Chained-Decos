#include "MapModule.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "Managers/MapManager.h"
#include "Engine/World/World.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Render/RenderManager.h"
#include "Game/Player/Player.h"
#include "Game/Menu/Menu.h"
#include <raylib.h>

MapModule::MapModule()
    : m_worldManager(nullptr), m_collisionManager(nullptr), m_modelLoader(nullptr), m_mapManager(nullptr)
{
}

bool MapModule::Initialize(Kernel* kernel)
{
    if (!kernel) {
        TraceLog(LOG_ERROR, "[MapModule] Kernel is null");
        return false;
    }

    // Отримуємо залежності з Kernel через сервіси
    auto worldService = kernel->GetService<WorldService>(Kernel::ServiceType::World);
    auto collisionService = kernel->GetService<CollisionService>(Kernel::ServiceType::Collision);
    auto modelsService = kernel->GetService<ModelsService>(Kernel::ServiceType::Models);

    if (worldService) {
        m_worldManager = worldService->world;
    }
    if (collisionService) {
        m_collisionManager = collisionService->cm;
    }
    if (modelsService) {
        m_modelLoader = modelsService->models;
    }

    // MapModule НЕ створює MapManager - він створюється в Game
    // MapModule тільки отримує посилання на існуючий MapManager з Kernel
    auto mapManagerService = kernel->GetService<MapManagerService>(Kernel::ServiceType::MapManager);
    if (mapManagerService) {
        m_mapManager = mapManagerService->mapManager;
        TraceLog(LOG_INFO, "[MapModule] MapManager obtained from Kernel");
    } else {
        TraceLog(LOG_WARNING, "[MapModule] MapManager not found in Kernel - it should be created by Game first");
        // MapManager буде створено Game пізніше, це нормально
    }

    TraceLog(LOG_INFO, "[MapModule] Initialized successfully");
    return true;
}

void MapModule::Shutdown()
{
    // MapModule не володіє MapManager, тому не видаляємо його
    m_mapManager = nullptr;
    m_worldManager = nullptr;
    m_collisionManager = nullptr;
    m_modelLoader = nullptr;
    
    TraceLog(LOG_INFO, "[MapModule] Shutdown complete");
}

void MapModule::Update(float deltaTime)
{
    // Map update logic if needed
}

void MapModule::Render()
{
    if (m_mapManager) {
        m_mapManager->RenderEditorMap();
        m_mapManager->RenderSpawnZone();
    }
}

void MapModule::RegisterServices(Kernel* kernel)
{
    // MapModule НЕ реєструє MapManager - він вже зареєстрований Game
    // Modules тільки використовують існуючі сервіси, не створюють нові
    (void)kernel;  // Unused
}

std::vector<std::string> MapModule::GetDependencies() const
{
    return {}; // Base module, no dependencies
}

