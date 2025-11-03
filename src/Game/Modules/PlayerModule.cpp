#include "PlayerModule.h"
#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "Engine/Engine.h"
#include "Game/Player/Player.h"
#include "Game/Managers/PlayerManager.h"
#include "Game/Managers/MapManager.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include <raylib.h>

PlayerModule::PlayerModule()
    : m_player(nullptr), m_playerManager(nullptr), m_collisionManager(nullptr), m_mapManager(nullptr)
{
}

bool PlayerModule::Initialize(Kernel* kernel)
{
    if (!kernel) {
        TraceLog(LOG_ERROR, "[PlayerModule] Kernel is null");
        return false;
    }

    // Отримуємо залежності з Kernel
    auto collisionService = kernel->GetService<CollisionService>(Kernel::ServiceType::Collision);
    auto mapService = kernel->GetService<MapManagerService>(Kernel::ServiceType::MapManager);
    auto modelsService = kernel->GetService<ModelsService>(Kernel::ServiceType::Models);
    
    // Отримуємо Engine з Kernel (якщо зареєстрований)
    Engine* engine = nullptr; // Поки що не реєструється як сервіс

    if (collisionService) {
        m_collisionManager = collisionService->cm;
    }
    if (mapService) {
        m_mapManager = mapService->mapManager;
    }

    // ResourceManager поки що не використовується в PlayerManager
    // Використовується ModelLoader замість нього
    ModelLoader* models = nullptr;
    if (modelsService) {
        models = modelsService->models;
    }

    // Перевіряємо обов'язкові залежності
    if (!m_collisionManager || !models || !m_mapManager) {
        TraceLog(LOG_WARNING, "[PlayerModule] Some dependencies not found in Kernel");
        // Можна продовжити з nullptr - залежить від реалізації
    }

    // PlayerModule НЕ створює Player/PlayerManager - вони створюються в Game
    // PlayerModule тільки отримує посилання на існуючі об'єкти з Kernel
    auto playerService = kernel->GetService<PlayerService>(Kernel::ServiceType::Player);
    auto playerManagerService = kernel->GetService<PlayerManagerService>(Kernel::ServiceType::PlayerManager);
    
    if (playerService) {
        m_player = playerService->player;
        TraceLog(LOG_INFO, "[PlayerModule] Player obtained from Kernel");
    } else {
        TraceLog(LOG_WARNING, "[PlayerModule] Player not found in Kernel - it should be created by Game first");
    }
    
    if (playerManagerService) {
        m_playerManager = playerManagerService->playerManager;
        TraceLog(LOG_INFO, "[PlayerModule] PlayerManager obtained from Kernel");
    } else {
        TraceLog(LOG_WARNING, "[PlayerModule] PlayerManager not found in Kernel - it should be created by Game first");
    }

    TraceLog(LOG_INFO, "[PlayerModule] Initialized successfully");
    return true;
}

void PlayerModule::Shutdown()
{
    // PlayerModule не володіє об'єктами, тому не видаляємо їх
    m_player = nullptr;
    m_playerManager = nullptr;
    m_collisionManager = nullptr;
    m_mapManager = nullptr;
    
    TraceLog(LOG_INFO, "[PlayerModule] Shutdown complete");
}

void PlayerModule::Update(float deltaTime)
{
    if (m_playerManager) {
        m_playerManager->UpdatePlayerLogic();
    }
}

void PlayerModule::Render()
{
    // Player rendering handled by GameRenderManager
    // This module focuses on logic only
}

void PlayerModule::RegisterServices(Kernel* kernel)
{
    // PlayerModule НЕ реєструє Player/PlayerManager - вони вже зареєстровані Game
    // Modules тільки використовують існуючі сервіси, не створюють нові
    (void)kernel;  // Unused
}

std::vector<std::string> PlayerModule::GetDependencies() const
{
    return {"Map"};
}

