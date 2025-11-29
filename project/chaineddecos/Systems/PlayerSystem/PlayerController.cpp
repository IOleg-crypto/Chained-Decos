#include "PlayerController.h"
#include "../MapSystem/LevelManager.h"
#include "core/engine/Engine.h"
#include "project/chaineddecos/Menu/Console/ConsoleManagerHelpers.h"
#include "project/chaineddecos/Menu/Menu.h"
#include "project/chaineddecos/Player/Core/Player.h"
#include "scene/resources/model/Core/Model.h"
#include "servers/physics/collision/Core/CollisionManager.h"
#include <imgui.h>
#include <raylib.h>

PlayerController::PlayerController()
    : m_player(nullptr), m_audioManager(nullptr), m_collisionManager(nullptr), m_mapSystem(nullptr),
      m_models(nullptr), m_engine(nullptr)
{
}

PlayerController::~PlayerController()
{
    Shutdown();
}

bool PlayerController::Initialize(Engine *engine)
{
    if (!engine)
    {
        TraceLog(LOG_ERROR, "[PlayerController] Engine is null");
        return false;
    }

    m_engine = engine;
    TraceLog(LOG_INFO, "[PlayerController] Initializing...");

    // Get engine dependencies through Engine
    m_collisionManager = engine->GetService<CollisionManager>().get();
    m_models = engine->GetService<ModelLoader>().get();
    m_audioManager = engine->GetService<AudioManager>().get();

    auto mapSystem = engine->GetLevelManager();

    // Validate required engine dependencies
    if (!m_collisionManager || !m_models || !m_audioManager)
    {
        TraceLog(LOG_ERROR, "[PlayerController] Required engine services not found");
        return false;
    }

    // LevelManager can be nullptr if LevelManager isn't initialized yet
    m_mapSystem = mapSystem;

    // Create our own components
    try
    {
        m_player = std::make_unique<Player>();
        TraceLog(LOG_INFO, "[PlayerController] Player created");

        // Inject AudioManager into Player
        if (m_audioManager)
        {
            m_player->SetAudioManager(
                std::shared_ptr<AudioManager>(m_audioManager, [](AudioManager *) {}));
            TraceLog(LOG_INFO, "[PlayerController] AudioManager injected into Player");
        }
        else
        {
            TraceLog(LOG_WARNING,
                     "[PlayerController] AudioManager is null, fall sounds will not work");
        }

        // Register services in Initialize so they're available to other systems
        RegisterServices(engine);

        TraceLog(LOG_INFO, "[PlayerController] Initialized successfully");
        return true;
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "[PlayerController] Failed to create components: %s", e.what());
        return false;
    }
}

void PlayerController::Shutdown()
{
    TraceLog(LOG_INFO, "[PlayerController] Shutting down...");

    // Clean up our own resources (we own them)
    m_player.reset();

    // Dependencies - references only, don't delete
    m_collisionManager = nullptr;
    m_mapSystem = nullptr;
    m_models = nullptr;
    m_engine = nullptr;

    TraceLog(LOG_INFO, "[PlayerController] Shutdown complete");
}

void PlayerController::Update(float deltaTime)
{
    // LevelManager should be available since PlayerController depends on LevelManager
    // But check anyway
    if (!m_mapSystem && m_engine)
    {
        auto mapSystem = m_engine->GetLevelManager();
        if (mapSystem)
        {
            m_mapSystem = mapSystem;
            TraceLog(LOG_INFO, "[PlayerController] LevelManager obtained from Engine");
        }
    }

    if (!m_player)
    {
        return;
    }

    // Check if player is at uninitialized position
    // Player starts at (-999999, -999999, -999999) until InitializePlayer() is called
    Vector3 pos = m_player->GetPlayerPosition();
    constexpr float UNINITIALIZED_THRESHOLD = -999000.0f;

    if (pos.y <= UNINITIALIZED_THRESHOLD || (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f))
    {
        // Player is not initialized yet - don't update
        return;
    }

    // Player has valid position - safe to update
    UpdatePlayerLogic();
}

void PlayerController::Render()
{
    // Player rendering handled by RenderingSystem::RenderGameWorld()
    // This system focuses on logic only, not rendering
}

void PlayerController::RegisterServices(Engine *engine)
{
    if (!engine)
    {
        return;
    }

    TraceLog(LOG_INFO, "[PlayerController] Registering services...");

    // Register PlayerController directly
    engine->RegisterService<PlayerController>(
        std::shared_ptr<PlayerController>(this, [](PlayerController *) {}));
    TraceLog(LOG_INFO, "[PlayerController] PlayerController registered");

    // Register Player directly (no wrapper needed)
    if (m_player)
    {
        engine->RegisterService<Player>(std::shared_ptr<Player>(m_player.get(), [](Player *) {}));
        TraceLog(LOG_INFO, "[PlayerController] Player registered");
    }
}

std::vector<std::string> PlayerController::GetDependencies() const
{
    // Depends on MapSystem (for MapManager)
    // Note: "Map" is the module name of MapSystem (GetModuleName returns "Map")
    return {"Map"};
}

void PlayerController::InitializePlayer()
{
    if (!m_player)
    {
        TraceLog(LOG_ERROR, "[PlayerController] Cannot initialize player - player is null");
        return;
    }

    // Set initial position on the first platform (mix of ground and floating platforms)
    Vector3 safePosition = {0.0f, PLAYER_SAFE_SPAWN_HEIGHT, 0.0f};
    TraceLog(
        LOG_INFO,
        "[PlayerController] InitializePlayer() - Setting initial safe position: (%.2f, %.2f, %.2f)",
        safePosition.x, safePosition.y, safePosition.z);
    m_player->SetPlayerPosition(safePosition);

    // Setup collision and physics
    TraceLog(LOG_INFO,
             "[PlayerController] InitializePlayer() - Setting up collision manager for player...");
    m_player->GetMovement()->SetCollisionManager(m_collisionManager);

    TraceLog(LOG_INFO, "[PlayerController] InitializePlayer() - Updating player collision box...");
    m_player->UpdatePlayerBox();

    TraceLog(LOG_INFO, "[PlayerController] InitializePlayer() - Updating player collision...");
    m_player->UpdatePlayerCollision();

    // Extract player from collider if stuck after spawn
    TraceLog(LOG_INFO,
             "[PlayerController] InitializePlayer() - Checking if player is stuck in collision...");
    if (m_player->GetMovement()->ExtractFromCollider())
    {
        TraceLog(LOG_INFO,
                 "[PlayerController] InitializePlayer() - Player extracted from collider");
    }

    // Allow physics to determine grounded state; start ungrounded so gravity applies
    TraceLog(LOG_INFO, "[PlayerController] InitializePlayer() - Setting initial physics state...");
    m_player->GetPhysics().SetGroundLevel(false);
    m_player->GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});

    // Load player model with improved error handling and fallback
    TraceLog(LOG_INFO, "[PlayerController] InitializePlayer() - Loading player model...");
    // Try to load the player model
    if (auto playerModel = m_models->GetModelByName("player_low"))
    {
        Model &model = playerModel->get();
        TraceLog(LOG_INFO,
                 "[PlayerController] InitializePlayer() - Player model pointer: %p, meshCount: %d",
                 &model, model.meshCount);

        if (model.meshCount > 0)
        {
            m_player->SetPlayerModel(&model);
            TraceLog(LOG_INFO,
                     "[PlayerController] InitializePlayer() - Player model loaded successfully.");
        }
        else
        {
            TraceLog(
                LOG_ERROR,
                "[PlayerController] InitializePlayer() - Player model is invalid or has no meshes");
            // Try to load player_low.glb directly if player.glb failed
            if (!m_models->LoadSingleModel("player", PROJECT_ROOT_DIR "/resources/player_low.glb",
                                           true))
            {
                TraceLog(LOG_ERROR, "[PlayerController] InitializePlayer() - Failed to load "
                                    "player_low.glb as fallback");
            }
            else
            {
                TraceLog(LOG_INFO, "[PlayerController] InitializePlayer() - Successfully loaded "
                                   "player_low.glb as fallback");
                if (auto playerModel = m_models->GetModelByName("player"))
                {
                    Model &model = playerModel->get();
                    if (model.meshCount > 0)
                    {
                        m_player->SetPlayerModel(&model);
                        TraceLog(LOG_INFO, "[PlayerController] InitializePlayer() - Player model "
                                           "loaded successfully with fallback.");
                    }
                }
            }
        }
    }

    TraceLog(LOG_INFO,
             "[PlayerController] InitializePlayer() - Player initialized at (%.2f, %.2f, %.2f).",
             safePosition.x, safePosition.y, safePosition.z);

    // Additional safety check - ensure player is properly positioned
    Vector3 currentPos = m_player->GetPlayerPosition();
    TraceLog(LOG_INFO,
             "[PlayerController] InitializePlayer() - Player current position: (%.2f, %.2f, %.2f)",
             currentPos.x, currentPos.y, currentPos.z);

    // Validate player position is safe (above ground but not too high)
    if (currentPos.y < 0.0f)
    {
        TraceLog(LOG_WARNING, "[PlayerController] InitializePlayer() - Player position below "
                              "ground level, adjusting");
        m_player->SetPlayerPosition({currentPos.x, PLAYER_SAFE_SPAWN_HEIGHT, currentPos.z});
    }
    else if (currentPos.y > 50.0f)
    {
        TraceLog(LOG_WARNING,
                 "[PlayerController] InitializePlayer() - Player position too high, adjusting");
        m_player->SetPlayerPosition({currentPos.x, PLAYER_SAFE_SPAWN_HEIGHT, currentPos.z});
    }

    // Check if map has player spawn objects and adjust position accordingly
    // First, look for objects with modelName == "player"
    // If not found, fall back to objects with "player_start" in name for backward compatibility
    TraceLog(LOG_INFO,
             "[PlayerController] InitializePlayer() - Checking for player spawn objects in map...");

    // No map objects, but check for spawn zone
    if (m_mapSystem && m_mapSystem->HasSpawnZone())
    {
        Vector3 spawnPos = m_mapSystem->GetPlayerSpawnPosition();
        m_player->DEFAULT_SPAWN_POSITION = spawnPos;
        m_player->SetPlayerPosition(spawnPos);
        TraceLog(
            LOG_INFO,
            "[PlayerController] InitializePlayer() - Using spawn zone position: (%.2f, %.2f, %.2f)",
            spawnPos.x, spawnPos.y, spawnPos.z);
    }
    else
    {
        TraceLog(LOG_INFO, "[PlayerController] InitializePlayer() - No map objects or spawn zone "
                           "found, using default position");
    }

    // Final position verification
    Vector3 finalPos = m_player->GetPlayerPosition();
    TraceLog(LOG_INFO,
             "[PlayerController] InitializePlayer() - Final player position: (%.2f, %.2f, %.2f)",
             finalPos.x, finalPos.y, finalPos.z);

    TraceLog(LOG_INFO, "[PlayerController] InitializePlayer() - Player initialization complete");
}

void PlayerController::UpdatePlayerLogic()
{
    if (!m_player)
    {
        return;
    }

    const ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        // Still update camera rotation even when ImGui wants mouse capture
        m_player->GetCameraController()->UpdateCameraRotation();
        m_player->GetCameraController()->UpdateMouseRotation(
            m_player->GetCameraController()->GetCamera(), m_player->GetMovement()->GetPosition());
        m_player->GetCameraController()->Update();

        // Show meters only if engine is available
        if (m_engine)
        {
            m_engine->GetRenderManager()->ShowMetersPlayer(*m_player->GetRenderable());
        }
    }

    m_player->Update(*m_collisionManager);

    // Show meters only if engine is available
    if (m_engine)
    {
        m_engine->GetRenderManager()->ShowMetersPlayer(*m_player->GetRenderable());
    }
}

void PlayerController::SavePlayerState(const std::string &currentMapPath)
{
    TraceLog(LOG_INFO, "[PlayerController] SavePlayerState() - Saving current game state...");

    if (!m_player)
    {
        TraceLog(LOG_ERROR, "[PlayerController] SavePlayerState() - Player is null");
        return;
    }

    m_savedMapPath = currentMapPath;
    TraceLog(LOG_INFO, "[PlayerController] SavePlayerState() - Saved map path: %s",
             m_savedMapPath.c_str());

    m_savedPlayerPosition = m_player->GetPlayerPosition();
    m_savedPlayerVelocity = m_player->GetPhysics().GetVelocity();
    TraceLog(LOG_INFO,
             "[PlayerController] SavePlayerState() - Saved player position: (%.2f, %.2f, %.2f)",
             m_savedPlayerPosition.x, m_savedPlayerPosition.y, m_savedPlayerPosition.z);

    // Enable resume button in menu
    if (m_engine)
    {
        // Access Menu via UIController
        // Since we don't have direct access to UIController here, and MenuService is gone,
        // we might need to rely on UIController polling or event system.
        // For now, let's try to get UIController module.
        if (auto moduleManager = m_engine->GetModuleManager())
        {
            if (auto uiModule = moduleManager->GetModule("UI"))
            {
                // We need to cast to UIController, but we need the header.
                // Since we don't want to include UIController.h here to avoid circular deps
                // (maybe), we can assume UIController handles this state or we can add a method to
                // IEngineModule or similar. Actually, let's just log for now.
                TraceLog(
                    LOG_INFO,
                    "[PlayerController] SavePlayerState() - TODO: Enable resume button in menu");
            }
        }
    }

    TraceLog(LOG_INFO, "[PlayerController] SavePlayerState() - Game state saved successfully");
}

void PlayerController::RestorePlayerState()
{
    TraceLog(LOG_INFO, "[PlayerController] RestorePlayerState() - Restoring game state...");

    if (!m_player)
    {
        TraceLog(LOG_ERROR, "[PlayerController] RestorePlayerState() - Player is null");
        return;
    }

    if (!m_savedMapPath.empty())
    {
        m_player->SetPlayerPosition(m_savedPlayerPosition);
        m_player->GetPhysics().SetVelocity(m_savedPlayerVelocity);
        TraceLog(LOG_INFO,
                 "[PlayerController] RestorePlayerState() - Restored player position: (%.2f, %.2f, "
                 "%.2f)",
                 m_savedPlayerPosition.x, m_savedPlayerPosition.y, m_savedPlayerPosition.z);
    }

    TraceLog(LOG_INFO,
             "[PlayerController] RestorePlayerState() - Game state restored successfully");
}
