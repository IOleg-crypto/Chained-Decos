#include "GameRenderManager.h"
#include "../Player/Player.h"
#include "Engine/Engine.h"
#include "Engine/Model/Model.h"
#include "Engine/Collision/CollisionManager.h"
#include "MapManager.h"
#include "GameRenderHelpers.h"
#include <raylib.h>

GameRenderManager::GameRenderManager(Player* player, Engine* engine, ModelLoader* models,
                                     CollisionManager* collisionManager, MapManager* mapManager)
    : m_player(player), m_engine(engine), m_models(models),
      m_collisionManager(collisionManager), m_mapManager(mapManager)
{
    TraceLog(LOG_INFO, "GameRenderManager created");
}

void GameRenderManager::RenderGameWorld()
{
    if (!m_engine)
    {
        TraceLog(LOG_WARNING,
                 "GameRenderManager::RenderGameWorld() - No engine provided, skipping game world render");
        return;
    }
    
    // Get camera from player
    Camera camera = m_player->GetCameraController()->GetCamera();
    
    // Begin 3D rendering
    BeginMode3D(camera);
    
    // Render editor-created map FIRST (primitives must be rendered before collision shapes)
    // to avoid collision wireframes covering primitives
    if (!m_mapManager->GetGameMap().objects.empty())
    {
        m_mapManager->RenderEditorMap();
    }
    
    // Render game world (models, player, etc.) and collision shapes AFTER primitives
    // This ensures primitives are visible and not covered by wireframes
    m_engine->GetRenderManager()->RenderGame(*m_player->GetRenderable(), *m_models, *m_collisionManager,
                                             m_engine->IsCollisionDebugVisible());
    
    // End 3D rendering
    EndMode3D();
}

void GameRenderManager::RenderGameUI() const
{
    if (!m_engine)
    {
        TraceLog(LOG_WARNING, "GameRenderManager::RenderGameUI() - No engine provided, skipping game UI render");
        return;
    }
    
    m_engine->GetRenderManager()->ShowMetersPlayer(*m_player->GetRenderable());
    
    static float gameTime = 0.0f;
    gameTime += GetFrameTime();
    
    int minutes = static_cast<int>(gameTime) / 60;
    int seconds = static_cast<int>(gameTime) % 60;
    int milliseconds =
        static_cast<int>((gameTime - static_cast<float>(static_cast<int>(gameTime))) * 1000);
    
    // Add timer icon using ASCII art timer (works on all systems)
    const char *timerIcon = "[TIMER] ";
    std::string timerText =
        TextFormat("%s%02d:%02d:%03d", timerIcon, minutes, seconds, milliseconds);
    
    Vector2 timerPos = {300.0f, 20.0f};
    
    Font fontToUse =
        (m_engine->GetRenderManager() && m_engine->GetRenderManager()->GetFont().texture.id != 0)
            ? m_engine->GetRenderManager()->GetFont()
            : GetFontDefault();
    
    float fontSize = GameRenderHelpers::CalculateDynamicFontSize(24.0f);
    DrawTextEx(fontToUse, timerText.c_str(), timerPos, fontSize, 2.0f, WHITE);
}

