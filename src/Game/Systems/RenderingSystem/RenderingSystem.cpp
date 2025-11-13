#include "RenderingSystem.h"
#include "Engine/Kernel/Core/Kernel.h"
#include "Engine/Kernel/Core/KernelServices.h"
#include "Engine/Engine.h"
#include "Engine/Render/Manager/RenderManager.h"
#include "../../Player/Player.h"
#include "../../Managers/MapManager.h"
#include "../../Managers/GameRenderHelpers.h"
#include "Engine/Collision/Manager/CollisionManager.h"
#include "Engine/Model/Core/Model.h"
#include "Engine/Map/Renderer/MapRenderer.h"
#include <raylib.h>

RenderingSystem::RenderingSystem()
    : m_kernel(nullptr),
      m_player(nullptr),
      m_mapManager(nullptr),
      m_collisionManager(nullptr),
      m_models(nullptr),
      m_engine(nullptr),
      m_gameTime(0.0f)
{
}

bool RenderingSystem::Initialize(Kernel* kernel)
{
    if (!kernel) {
        TraceLog(LOG_ERROR, "[RenderingSystem] Kernel is null");
        return false;
    }

    m_kernel = kernel;
    TraceLog(LOG_INFO, "[RenderingSystem] Initializing...");
    
    // Lazy loading: don't get services now, only when first used
    // Allows RenderingSystem to initialize even if other systems haven't registered services yet
    
    TraceLog(LOG_INFO, "[RenderingSystem] Initialized successfully (services will be loaded on first use)");
    return true;
}

void RenderingSystem::EnsureDependencies()
{
    if (!m_kernel) {
        TraceLog(LOG_ERROR, "[RenderingSystem] Kernel is null, cannot load dependencies");
        return;
    }

    if (!m_models) {
        auto modelsService = m_kernel->GetService<ModelsService>();
        if (modelsService) {
            m_models = modelsService->models;
        }
    }
    // Get dependencies through Kernel (lazy loading)
    if (!m_player) {
        auto playerService = m_kernel->GetService<PlayerService>();
        if (playerService) {
            m_player = playerService->player;
        }
    }
    
    if (!m_mapManager) {
        auto mapService = m_kernel->GetService<MapManagerService>();
        if (mapService) {
            m_mapManager = mapService->mapManager;
        }
    }
    
    if (!m_collisionManager) {
        auto collisionService = m_kernel->GetService<CollisionService>();
        if (collisionService) {
            m_collisionManager = collisionService->cm;
        }
    }
    
    
    if (!m_engine) {
        auto engineService = m_kernel->GetService<EngineService>();
        if (engineService) {
            m_engine = engineService->engine;
        }
    }
}

void RenderingSystem::Shutdown()
{
    TraceLog(LOG_INFO, "[RenderingSystem] Shutting down...");
    
    m_kernel = nullptr;
    m_player = nullptr;
    m_mapManager = nullptr;
    m_collisionManager = nullptr;
    m_models = nullptr;
    m_engine = nullptr;
    m_gameTime = 0.0f;
    
    TraceLog(LOG_INFO, "[RenderingSystem] Shutdown complete");
}

void RenderingSystem::Update(float deltaTime)
{
    // Update timer for UI
    m_gameTime += deltaTime;
}

void RenderingSystem::Render()
{
    // RenderingSystem::Render is called through ModuleManager::RenderAllModules()
    // But for correct rendering order (inside BeginMode3D/EndMode3D)
    // rendering is done in GameApplication::OnPostRender() through RenderGameWorld()
}

void RenderingSystem::RegisterServices(Kernel* kernel)
{
    // RenderingSystem doesn't register services - it only renders
    // Components it renders are already registered by other systems
    (void)kernel;  // Suppress unused parameter warning
}

std::vector<std::string> RenderingSystem::GetDependencies() const
{
    // Depends on PlayerSystem and MapSystem
    return {"Player", "Map"};
}

void RenderingSystem::RenderGameWorld()
{
    // Lazy load dependencies
    EnsureDependencies();
    
    if (!m_engine || !m_player || !m_mapManager) {
        TraceLog(LOG_WARNING, "[RenderingSystem] Missing dependencies for RenderGameWorld");
        return;
    }
    
    // Get camera from player
    Camera camera = m_player->GetCameraController()->GetCamera();
    
    // Render editor-created map using MapRenderer (includes skybox)
    // Note: RenderMap handles BeginMode3D/EndMode3D internally
    GameMap& gameMap = m_mapManager->GetGameMap();
    if (!gameMap.GetMapObjects().empty()) {
        MapRenderer renderer;
        renderer.RenderMap(gameMap, camera);
    }
    
    // Render spawn zone (commented out - only in Map Editor)
    // m_mapManager->RenderSpawnZone();
    
    // Begin 3D rendering for game world elements
    BeginMode3D(camera);
    
    // Render game world (models, player, etc.) and collision shapes AFTER primitives
    m_engine->GetRenderManager()->RenderGame(*m_player->GetRenderable(), *m_models, *m_collisionManager,
                                             m_engine->IsCollisionDebugVisible());
    
    // End 3D rendering
    EndMode3D();
}

void RenderingSystem::RenderGameUI() const
{
    // Lazy load dependencies (const version)
    const_cast<RenderingSystem*>(this)->EnsureDependencies();
    
    if (!m_engine || !m_player) {
        TraceLog(LOG_WARNING, "[RenderingSystem] Missing dependencies for RenderGameUI");
        return;
    }
    
    m_engine->GetRenderManager()->ShowMetersPlayer(*m_player->GetRenderable());
    
    int minutes = static_cast<int>(m_gameTime) / 60;
    int seconds = static_cast<int>(m_gameTime) % 60;
    int milliseconds = static_cast<int>((m_gameTime - static_cast<float>(static_cast<int>(m_gameTime))) * 1000);
    
    // Timer icon and text
    const char* timerIcon = "[TIMER] ";
    std::string timerText = TextFormat("%s%02d:%02d:%03d", timerIcon, minutes, seconds, milliseconds);
    
    Vector2 timerPos = {300.0f, 20.0f};
    
    Font fontToUse = (m_engine->GetRenderManager() && m_engine->GetRenderManager()->GetFont().texture.id != 0)
        ? m_engine->GetRenderManager()->GetFont()
        : GetFontDefault();
    
    float fontSize = GameRenderHelpers::CalculateDynamicFontSize(24.0f);
    DrawTextEx(fontToUse, timerText.c_str(), timerPos, fontSize, 2.0f, WHITE);
}

