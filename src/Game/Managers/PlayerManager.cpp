#include "PlayerManager.h"
#include "../Player/Player.h"
#include "Engine/Collision/CollisionManager.h"
#include "Engine/Model/Model.h"
#include "Engine/Engine.h"
#include "Engine/Map/MapLoader.h"
#include "MapManager.h"
#include <raylib.h>
#include <imgui.h>

PlayerManager::PlayerManager(Player* player, CollisionManager* collisionManager,
                             ModelLoader* models, Engine* engine, MapManager* mapManager)
    : m_player(player), m_collisionManager(collisionManager), 
      m_models(models), m_engine(engine), m_mapManager(mapManager)
{
    TraceLog(LOG_INFO, "PlayerManager created");
}

void PlayerManager::InitPlayer()
{
    // Set initial position on the first platform (mix of ground and floating platforms)
    Vector3 safePosition = {0.0f, PLAYER_SAFE_SPAWN_HEIGHT, 0.0f};
    TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Setting initial safe position: (%.2f, %.2f, %.2f)",
             safePosition.x, safePosition.y, safePosition.z);
    m_player->SetPlayerPosition(safePosition);

    // Setup collision and physics
    TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Setting up collision manager for player...");
    m_player->GetMovement()->SetCollisionManager(m_collisionManager);
    
    TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Updating player collision box...");
    m_player->UpdatePlayerBox();
    
    TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Updating player collision...");
    m_player->UpdatePlayerCollision();
    
    // Allow physics to determine grounded state; start ungrounded so gravity applies
    TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Setting initial physics state...");
    m_player->GetPhysics().SetGroundLevel(false);
    m_player->GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});
    
    // Load player model with improved error handling and fallback
    TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Loading player model...");
    // Try to load the player model
    if (auto playerModel = m_models->GetModelByName("player_low"))
    {
        Model &model = playerModel->get();
        TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Player model pointer: %p, meshCount: %d", &model,
                 model.meshCount);
        
        if (model.meshCount > 0)
        {
            m_player->SetPlayerModel(&model);
            TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Player model loaded successfully.");
        }
        else
        {
            TraceLog(LOG_ERROR, "PlayerManager::InitPlayer() - Player model is invalid or has no meshes");
            // Try to load player_low.glb directly if player.glb failed
            if (!m_models->LoadSingleModel("player", PROJECT_ROOT_DIR "/resources/player_low.glb", true))
            {
                TraceLog(LOG_ERROR,
                         "PlayerManager::InitPlayer() - Failed to load player_low.glb as fallback");
            }
            else
            {
                TraceLog(LOG_INFO,
                         "PlayerManager::InitPlayer() - Successfully loaded player_low.glb as fallback");
                if (auto playerModel = m_models->GetModelByName("player"))
                {
                    Model &model = playerModel->get();
                    if (model.meshCount > 0)
                    {
                        m_player->SetPlayerModel(&model);
                        TraceLog(
                            LOG_INFO,
                            "PlayerManager::InitPlayer() - Player model loaded successfully with fallback.");
                    }
                }
            }
        }
    }
    
    TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Player initialized at (%.2f, %.2f, %.2f).",
             safePosition.x, safePosition.y, safePosition.z);
    
    // Additional safety check - ensure player is properly positioned
    Vector3 currentPos = m_player->GetPlayerPosition();
    TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Player current position: (%.2f, %.2f, %.2f)",
             currentPos.x, currentPos.y, currentPos.z);
    
    // Validate player position is safe (above ground but not too high)
    if (currentPos.y < 0.0f)
    {
        TraceLog(LOG_WARNING, "PlayerManager::InitPlayer() - Player position below ground level, adjusting");
        m_player->SetPlayerPosition({currentPos.x, PLAYER_SAFE_SPAWN_HEIGHT, currentPos.z});
    }
    else if (currentPos.y > 50.0f)
    {
        TraceLog(LOG_WARNING, "PlayerManager::InitPlayer() - Player position too high, adjusting");
        m_player->SetPlayerPosition({currentPos.x, PLAYER_SAFE_SPAWN_HEIGHT, currentPos.z});
    }
    
    // Check if map has PlayerStart objects and adjust position accordingly
    TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Checking for PlayerStart objects in map...");
    if (!m_mapManager->GetGameMap().objects.empty())
    {
        TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Map has %d objects, searching for PlayerStart...",
                 m_mapManager->GetGameMap().objects.size());
        for (size_t i = 0; i < m_mapManager->GetGameMap().objects.size(); ++i)
        {
            const auto &obj = m_mapManager->GetGameMap().objects[i];
            TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Checking object %d: %s (type: %d)", i,
                     obj.name.c_str(), static_cast<int>(obj.type));
            
            if ((obj.type == MapObjectType::MODEL || obj.type == MapObjectType::LIGHT) &&
                obj.name.find("player_start") != std::string::npos)
            {
                TraceLog(LOG_INFO,
                         "PlayerManager::InitPlayer() - Found PlayerStart object at (%.2f, %.2f, %.2f)",
                         obj.position.x, obj.position.y, obj.position.z);
                m_player->SetPlayerPosition(obj.position);
                TraceLog(LOG_INFO,
                         "PlayerManager::InitPlayer() - Player position updated to PlayerStart location");
                break;
            }
        }
    }
    else
    {
        TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - No map objects found, using default position");
    }
    
    // Final position verification
    Vector3 finalPos = m_player->GetPlayerPosition();
    TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Final player position: (%.2f, %.2f, %.2f)", finalPos.x,
             finalPos.y, finalPos.z);
    
    TraceLog(LOG_INFO, "PlayerManager::InitPlayer() - Player initialization complete");
}

void PlayerManager::UpdatePlayerLogic()
{
    if (!m_engine)
    {
        // Skip player logic if no engine is available (for testing)
        m_player->Update(*m_collisionManager);
        return;
    }
    
    const ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        // Still update camera rotation even when ImGui wants mouse capture
        // This allows camera to work when menu is open or when hovering over UI
        m_player->GetCameraController()->UpdateCameraRotation();
        m_player->GetCameraController()->UpdateMouseRotation(
            m_player->GetCameraController()->GetCamera(), m_player->GetMovement()->GetPosition());
        m_player->GetCameraController()->Update();
        
        m_engine->GetRenderManager()->ShowMetersPlayer(*m_player->GetRenderable());
        // return;
    }
    
    m_player->Update(*m_collisionManager);
    
    m_engine->GetRenderManager()->ShowMetersPlayer(*m_player->GetRenderable());
}

