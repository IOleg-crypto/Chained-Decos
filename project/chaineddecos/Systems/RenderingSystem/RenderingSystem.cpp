#include "RenderingSystem.h"
#include "../MapSystem/MapSystem.h"
#include "core/object/kernel/Core/Kernel.h"
#include "platform/windows/Core/EngineApplication.h"
#include "project/chaineddecos/Player/Core/Player.h"
#include "servers/rendering/Core/RenderManager.h"

#include "scene/resources/map/Renderer/MapRenderer.h"
#include "scene/resources/model/Core/Model.h"
#include "servers/physics/collision/Core/CollisionManager.h"
#include <raylib.h>

RenderingSystem::RenderingSystem()
    : m_kernel(nullptr), m_player(nullptr), m_mapSystem(nullptr), m_collisionManager(nullptr),
      m_models(nullptr), m_engine(nullptr), m_gameTime(0.0f)
{
}

bool RenderingSystem::Initialize(Kernel *kernel)
{
    if (!kernel)
    {
        TraceLog(LOG_ERROR, "[RenderingSystem] Kernel is null");
        return false;
    }

    m_kernel = kernel;
    TraceLog(LOG_INFO, "[RenderingSystem] Initializing...");

    // Lazy loading: don't get services now, only when first used
    // Allows RenderingSystem to initialize even if other systems haven't registered services yet

    TraceLog(LOG_INFO,
             "[RenderingSystem] Initialized successfully (services will be loaded on first use)");
    return true;
}

void RenderingSystem::EnsureDependencies()
{
    if (!m_kernel)
    {
        TraceLog(LOG_ERROR, "[RenderingSystem] Kernel is null, cannot load dependencies");
        return;
    }

    if (!m_models)
    {
        auto modelLoader = m_kernel->GetService<ModelLoader>();
        if (modelLoader)
        {
            m_models = modelLoader.get();
        }
    }
    // Get dependencies through Kernel (lazy loading)
    if (!m_player)
    {
        auto playerService = m_kernel->GetService<PlayerService>();
        if (playerService)
        {
            m_player = playerService->player;
        }
    }

    if (!m_mapSystem)
    {
        auto mapSystemService = m_kernel->GetService<MapSystemService>();
        if (mapSystemService)
        {
            m_mapSystem = mapSystemService->mapSystem;
        }
    }

    if (!m_collisionManager)
    {
        auto collisionManager = m_kernel->GetService<CollisionManager>();
        if (collisionManager)
        {
            m_collisionManager = collisionManager.get();
        }
    }

    if (!m_engine)
    {
        auto engineObj = m_kernel->GetObject<Engine>();
        if (engineObj)
        {
            m_engine = engineObj.get();
        }
    }
}

void RenderingSystem::Shutdown()
{
    TraceLog(LOG_INFO, "[RenderingSystem] Shutting down...");

    m_kernel = nullptr;
    m_player = nullptr;
    m_mapSystem = nullptr;
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

void RenderingSystem::RegisterServices(Kernel *kernel)
{
    // RenderingSystem doesn't register services - it only renders
    // Components it renders are already registered by other systems
    (void)kernel; // Suppress unused parameter warning
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

    if (!m_engine || !m_player || !m_mapSystem)
    {
        TraceLog(LOG_WARNING, "[RenderingSystem] Missing dependencies for RenderGameWorld");
        return;
    }

    // Get camera from player
    Camera camera = m_player->GetCameraController()->GetCamera();

    // Render editor-created map using MapRenderer (includes skybox)
    // Note: RenderMap handles BeginMode3D/EndMode3D internally
    GameMap &gameMap = m_mapSystem->GetGameMap();
    if (!gameMap.GetMapObjects().empty())
    {
        MapRenderer renderer;
        renderer.RenderMap(gameMap, camera);
    }

    // Render spawn zone (commented out - only in Map Editor)
    // m_mapSystem->RenderSpawnZone();

    // Begin 3D rendering for game world elements
    BeginMode3D(camera);

    // Render game world (models, player, etc.) and collision shapes AFTER primitives
    m_engine->GetRenderManager()->RenderGame(
        *m_player->GetRenderable(), *m_models, *m_collisionManager,
        m_engine->GetRenderManager()->IsCollisionDebugVisible());

    // End 3D rendering
    EndMode3D();
}

void RenderingSystem::RenderGameUI() const
{
    // Lazy load dependencies (const version)
    const_cast<RenderingSystem *>(this)->EnsureDependencies();

    if (!m_engine || !m_player)
    {
        TraceLog(LOG_WARNING, "[RenderingSystem] Missing dependencies for RenderGameUI");
        return;
    }

    m_engine->GetRenderManager()->ShowMetersPlayer(*m_player->GetRenderable());

    int minutes = static_cast<int>(m_gameTime) / 60;
    int seconds = static_cast<int>(m_gameTime) % 60;
    int milliseconds =
        static_cast<int>((m_gameTime - static_cast<float>(static_cast<int>(m_gameTime))) * 1000);

    // Timer icon and text
    const char *timerIcon = "[TIMER] ";
    std::string timerText =
        TextFormat("%s%02d:%02d:%03d", timerIcon, minutes, seconds, milliseconds);

    Vector2 timerPos = {300.0f, 20.0f};

    Font fontToUse =
        (m_engine->GetRenderManager() && m_engine->GetRenderManager()->GetFont().texture.id != 0)
            ? m_engine->GetRenderManager()->GetFont()
            : GetFontDefault();

    // Calculate dynamic font size based on screen width
    int screenWidth = GetScreenWidth();
    float fontSize = 24.0f * (screenWidth / 1920.0f);
    fontSize = std::max(16.0f, std::min(fontSize, 48.0f));
    DrawTextEx(fontToUse, timerText.c_str(), timerPos, fontSize, 2.0f, WHITE);
}
