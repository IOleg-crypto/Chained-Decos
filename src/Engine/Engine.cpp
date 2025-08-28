//
// Engine.cpp - Main Engine Implementation
//

#include "Engine.h"

// Standard library
#include <set>

// Raylib & ImGui
#include <Menu/Menu.h>
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>
#include <rcamera.h>
#include <rlImGui.h>

// Collision system
#include <Collision/CollisionSystem.h>

Engine::Engine() : Engine(800, 600) {}

Engine::Engine(const int screenX, const int screenY)
    : m_screenX(screenX), m_screenY(screenY), m_windowName("Chained Decos"), m_menu(),
      m_showMenu(true), m_shouldExit(false), m_windowInitialized(false), m_showDebug(false),
      m_showCollisionDebug(false), m_isEngineInit(false)
{
    // Improved validation using local constants
    constexpr int DEFAULT_SCREEN_WIDTH = 800;
    constexpr int DEFAULT_SCREEN_HEIGHT = 600;

    if (m_screenX <= 0 || m_screenY <= 0)
    {
        TraceLog(LOG_WARNING, "[Screen] Invalid screen size: %d x %d. Setting default size %dx%d.",
                 m_screenX, m_screenY, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT);
        m_screenX = DEFAULT_SCREEN_WIDTH;
        m_screenY = DEFAULT_SCREEN_HEIGHT;
    }

    TraceLog(LOG_INFO, "Engine initialized with screen size: %dx%d", m_screenX, m_screenY);
}

Engine::~Engine()
{
    TraceLog(LOG_INFO, "Starting Engine destructor...");

    // Clear collision system first to avoid dangling pointers
    TraceLog(LOG_INFO, "Clearing collision manager...");
    m_collisionManager.ClearColliders();

    TraceLog(LOG_INFO, "Shutting down ImGui...");
    rlImGuiShutdown();

    // Only close window if this Engine instance initialized it
    if (m_windowInitialized && IsWindowReady())
    {
        TraceLog(LOG_INFO, "Closing window...");
        CloseWindow();
    }

    TraceLog(LOG_INFO, "Engine destructor completed");
}

// ==================== MAIN API ====================

void Engine::Init()
{
    TraceLog(LOG_INFO, "Initializing Engine...");

    m_menu.GetEngine(this);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(m_screenX, m_screenY, m_windowName.c_str());
    m_windowInitialized = true;
    SetTargetFPS(60);
    SetExitKey(KEY_NULL); //  To avoid closing the window - escape key

    m_renderManager.Initialize();

    const std::string modelsJsonPath = PROJECT_ROOT_DIR "/src/models.json";
    m_models.SetCacheEnabled(true);
    m_models.SetMaxCacheSize(50);
    m_models.EnableLOD(true);

    try
    {
        m_models.LoadModelsFromJson(modelsJsonPath);
        m_models.PrintStatistics();
        TraceLog(LOG_INFO, "Models loaded successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Failed to load models: %s", e.what());
    }

    InitCollisions();
    InitInput();

    m_player.GetMovement()->SetCollisionManager(&m_collisionManager);
    m_player.UpdatePlayerBox();
    m_player.UpdatePlayerCollision();

    m_isEngineInit = true;

    if (!m_collisionManager.GetColliders().empty())
    {

        Vector3 groundTop = PhysicsComponent::GROUND_COLLISION_CENTER;
        groundTop.y += PhysicsComponent::GROUND_COLLISION_SIZE.y / 2.0f;

        Vector3 safePosition = {0.0f, groundTop.y + 0.01f, 0.0f};
        m_player.SetPlayerPosition(safePosition);

        m_player.UpdatePlayerBox();
        m_player.UpdatePlayerCollision();

        m_player.GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});
        m_player.GetPhysics().SetGroundLevel(false);

        m_player.SnapToGroundIfNeeded(m_collisionManager);

        TraceLog(LOG_INFO, "Player positioned safely at: (%.2f, %.2f, %.2f)", safePosition.x,
                 safePosition.y, safePosition.z);
    }
    else
    {
        TraceLog(LOG_WARNING, "No colliders found! Creating emergency ground plane.");

        Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
        Vector3 groundSize = PhysicsComponent::GROUND_COLLISION_SIZE;
        Collision plane{groundCenter, groundSize};
        plane.SetCollisionType(CollisionType::AABB_ONLY);
        m_collisionManager.AddCollider(plane);

        Vector3 safePosition = {0.0f, groundCenter.y + groundSize.y / 2.0f + 0.01f, 0.0f};
        m_player.SetPlayerPosition(safePosition);
        m_player.GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});
        m_player.GetPhysics().SetGroundLevel(false);
        m_player.UpdatePlayerBox();
        m_player.UpdatePlayerCollision();
    }

    TraceLog(LOG_INFO, "Engine initialization complete!");
}

void Engine::Run()
{
    TraceLog(LOG_INFO, "Starting main game loop...");

    while (!WindowShouldClose() && !m_shouldExit)
    {
        Update();
        Render();
    }

    if (m_shouldExit)
        TraceLog(LOG_INFO, "Main loop exiting: exit requested (F4 or code)");
    else if (WindowShouldClose())
        TraceLog(LOG_INFO, "Main loop exiting: window closed (ESC/close button)");

    TraceLog(LOG_INFO, "Main game loop ended");
}

void Engine::ToggleMenu()
{
    m_showMenu = !m_showMenu;
    TraceLog(LOG_INFO, "Menu toggled: %s", m_showMenu ? "ON" : "OFF");
}

void Engine::RequestExit()
{
    m_shouldExit = true;
    TraceLog(LOG_INFO, "Exit requested");
}

bool Engine::IsRunning() const { return !WindowShouldClose() && !m_shouldExit; }

// ==================== INITIALIZATION METHODS ====================

void Engine::InitInput()
{
    TraceLog(LOG_INFO, "Setting up input bindings...");

    // Function keys - UI control
    m_manager.RegisterAction(KEY_F11, []() { ToggleFullscreen(); });
    m_manager.RegisterAction(KEY_F1,
                             [this]()
                             {
                                 m_showMenu = true;
                                 EnableCursor();
                             });
    m_manager.RegisterAction(KEY_F4, [this]() { m_shouldExit = true; });

    // Debug visualization toggles
    m_manager.RegisterAction(KEY_F2,
                             [this]()
                             {
                                 m_renderManager.ToggleDebugInfo();
                                 m_showDebug = m_renderManager.IsDebugInfoVisible();
                                 TraceLog(LOG_INFO, "Debug info: %s", m_showDebug ? "ON" : "OFF");
                             });

    m_manager.RegisterAction(KEY_F3,
                             [this]()
                             {
                                 m_renderManager.ToggleCollisionDebug();
                                 m_showCollisionDebug = m_renderManager.IsCollisionDebugVisible();
                                 TraceLog(LOG_INFO, "Collision debug: %s",
                                          m_showCollisionDebug ? "ON" : "OFF");
                             });

    // Utility functions
    m_manager.RegisterAction(KEY_F8, [this]() { OptimizeModelPerformance(); });
    m_manager.RegisterAction(
        KEY_F10,
        [this]()
        {
            TraceLog(LOG_INFO, "F10 pressed - forcing collision debug ON for next frame");
            m_renderManager.ForceCollisionDebugNextFrame();
        });

    // Model information display
    m_manager.RegisterAction(KEY_F9,
                             [this]()
                             {
                                 auto models = m_models.GetAvailableModels();
                                 TraceLog(LOG_INFO, "=== Available Models ===");
                                 for (const auto &model : models)
                                 {
                                     TraceLog(LOG_INFO, "  - %s", model.c_str());
                                 }

                                 auto decorations = m_models.GetInstancesByTag("decoration");
                                 TraceLog(LOG_INFO, "=== Instances by Tags ===");
                                 TraceLog(LOG_INFO, "  - Decorations: %zu", decorations.size());
                             });
    m_manager.RegisterAction(KEY_ESCAPE,
                             [this]()
                             {
                                 // Open menu only if currently in game; do not close menu with ESC
                                 if (!m_showMenu)
                                 {
                                     ToggleMenu();
                                     EnableCursor();
                                 }
                                 // If menu is open, let Menu handle ESC navigation/back behavior
                             });

    TraceLog(LOG_INFO, "Input bindings configured successfully");
}
void Engine::InitCollisions()
{
    TraceLog(LOG_INFO, "Starting collision system initialization...");

    // Clear any existing colliders first
    m_collisionManager.ClearColliders();
    TraceLog(LOG_INFO, "Cleared existing colliders");

    // Step 1: Create basic ground plane
    Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
    Vector3 groundSize = PhysicsComponent::GROUND_COLLISION_SIZE;
    Collision groundPlane{groundCenter, groundSize};
    groundPlane.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(groundPlane);

    TraceLog(LOG_INFO, "Added ground plane at (%.1f, %.1f, %.1f) with size (%.1f, %.1f, %.1f)",
             groundCenter.x, groundCenter.y, groundCenter.z, groundSize.x, groundSize.y,
             groundSize.z);

    // Verify ground plane was added
    TraceLog(LOG_INFO, "Collider count after adding ground plane: %zu",
             m_collisionManager.GetColliders().size());

    // Step 2: Create collisions for all models with hasCollision=true
    m_collisionManager.CreateAutoCollisionsFromModels(m_models);

    // Force collision system to initialize
    m_collisionManager.Initialize();

    TraceLog(LOG_INFO, "Collision system initialization complete with %zu colliders",
             m_collisionManager.GetColliders().size());

    // Log collision system state
    TraceLog(LOG_INFO, "Collision system initialized with %zu colliders",
             m_collisionManager.GetColliders().size());

    // Verify colliders exist
    if (m_collisionManager.GetColliders().empty())
    {
        TraceLog(LOG_ERROR, "CRITICAL ERROR: No colliders were created during initialization!");
    }
}

// ==================== UPDATE METHODS ====================

void Engine::Update()
{
    HandleKeyboardShortcuts();

    // Update game systems only if not in menu
    if (!m_showMenu)
    {
        // Verify collision system has colliders before updating physics
        if (m_collisionManager.GetColliders().empty())
        {
            static bool warningShown = false;
            if (!warningShown)
            {
                TraceLog(LOG_ERROR,
                         "CRITICAL ERROR: No colliders available for physics in Update!");
                warningShown = true;
            }

            // Create emergency ground plane if no colliders exist
            Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
            Vector3 groundSize = PhysicsComponent::GROUND_COLLISION_SIZE;
            Collision plane{groundCenter, groundSize};
            plane.SetCollisionType(CollisionType::AABB_ONLY);
            m_collisionManager.AddCollider(plane);

            TraceLog(LOG_WARNING,
                     "Created emergency ground plane in Update with size: {%.1f, %.1f, %.1f} at "
                     "position: {%.1f, %.1f, %.1f}",
                     groundSize.x, groundSize.y, groundSize.z, groundCenter.x, groundCenter.y,
                     groundCenter.z);
        }

        // Update player and physics with safety checks
        try
        {
            UpdatePlayer();
            UpdatePhysics();
            // CheckPlayerBounds(); // Temporarily disabled for debugging
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Exception during update: %s", e.what());

            // Reset player to safe position
            Vector3 safePosition = {0.0f, 2.0f, 0.0f};
            m_player.SetPlayerPosition(safePosition);
            m_player.GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});
            m_player.GetPhysics().SetGroundLevel(false);
        }
    }

    // --- Handle menu actions ---
    switch (m_menu.GetAction())
    {
    case MenuAction::SinglePlayer:
    {
        m_showMenu = false;
        TraceLog(LOG_INFO, "Starting singleplayer...");

        InitCollisions();
        InitInput();
        HideCursor();

        m_player.SetPlayerPosition(m_player.DEFAULT_SPAWN_POSITION);
        m_player.GetPhysics().SetVelocity({0, 0, 0});
        m_player.UpdatePlayerBox();
        m_player.UpdatePlayerCollision();

        if (m_isEngineInit)
        {
            m_player.GetPhysics().SetGroundLevel(false);
        }
        else
        {
            m_player.GetPhysics().SetGroundLevel(false);
        }
        if (!m_collisionManager.GetColliders().empty())
        {
            TraceLog(LOG_INFO, "Applying initial gravity with %zu colliders",
                     m_collisionManager.GetColliders().size());

            m_player.ApplyGravityForPlayer(m_collisionManager);

            Vector3 pos = m_player.GetPlayerPosition();
            TraceLog(LOG_INFO, "Initial gravity applied, pos: (%.2f, %.2f, %.2f)", pos.x, pos.y,
                     pos.z);
        }
        else
        {
            TraceLog(LOG_ERROR, "No colliders found, creating emergency ground plane");

            Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
            Vector3 groundSize = PhysicsComponent::GROUND_COLLISION_SIZE;

            Collision plane{groundCenter, groundSize};
            plane.SetCollisionType(CollisionType::AABB_ONLY);
            m_collisionManager.AddCollider(plane);

            TraceLog(
                LOG_WARNING,
                "Emergency ground plane created size: {%.1f, %.1f, %.1f} pos: {%.1f, %.1f, %.1f}",
                groundSize.x, groundSize.y, groundSize.z, groundCenter.x, groundCenter.y,
                groundCenter.z);
        }

        m_menu.ResetAction();
        break;
    }

    case MenuAction::OpenOptions:
        TraceLog(LOG_INFO, "Options menu opened");
        m_menu.ResetAction();
        break;

    case MenuAction::ExitGame:
        TraceLog(LOG_INFO, "Exit requested");
        m_shouldExit = true;
        m_menu.ResetAction();
        break;

    default:
        break;
    }
}

void Engine::UpdatePlayer()
{
    // Skip update if ImGui is capturing mouse input
    const ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        m_renderManager.ShowMetersPlayer(m_player);
        return;
    }

    // Update player position and state
    // Player::Update() now handles everything including collision via StepMovement
    m_player.Update(m_collisionManager);

    // Update UI meters
    m_renderManager.ShowMetersPlayer(m_player);
}

// Helper method to handle player collision
void Engine::HandlePlayerCollision()
{
    // Ensure collision box is updated with current player position
    m_player.UpdatePlayerBox();

    // Verify collision system has colliders
    if (m_collisionManager.GetColliders().empty())
    {
        static bool warningShown = false;
        if (!warningShown)
        {
            TraceLog(LOG_ERROR, "CRITICAL ERROR: No colliders available for collision detection!");
            warningShown = true;
        }
        return;
    }

    // Check for collisions and get response vector
    Vector3 response = {};
    bool isColliding = m_collisionManager.CheckCollision(m_player.GetCollision(), response);

    // Apply collision response if needed
    if (isColliding)
    {
        TraceLog(LOG_INFO, "Collision detected, applying response: (%.2f, %.2f, %.2f)", response.x,
                 response.y, response.z);
        m_player.Move(response);
        // m_player.GetPhysics().SetVelocity(Vector3Zero());
    }
}

void Engine::UpdatePhysics()
{
    // Ensure at least a basic ground plane exists
    if (m_collisionManager.GetColliders().empty())
    {
        static bool warningShown = false;
        if (!warningShown)
        {
            TraceLog(LOG_ERROR, "CRITICAL ERROR: No colliders available for physics!");
            warningShown = true;
        }

        // Create emergency ground plane if no colliders exist
        Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
        Vector3 groundSize = PhysicsComponent::GROUND_COLLISION_SIZE;
        Collision plane{groundCenter, groundSize};
        plane.SetCollisionType(CollisionType::AABB_ONLY);
        m_collisionManager.AddCollider(plane);

        TraceLog(LOG_WARNING,
                 "Created emergency ground plane with size: {%.1f, %.1f, %.1f} at position: {%.1f, "
                 "%.1f, %.1f}, collider count now: %zu",
                 groundSize.x, groundSize.y, groundSize.z, groundCenter.x, groundCenter.y,
                 groundCenter.z, m_collisionManager.GetColliders().size());
    }

    // Player physics are handled in UpdatePlayer()
}

void Engine::CheckPlayerBounds()
{
    EnsureGroundPlaneExists();

    Vector3 playerPos = m_player.GetPlayerPosition();
    Vector3 playerVel = m_player.GetPhysics().GetVelocity();

    if (IsPlayerOutOfBounds(playerPos) || m_physics.HasExtremeVelocity(playerVel))
    {
        TracePlayerIssue(playerPos, playerVel);

        Vector3 safeSpawn = {0.0f, 5.0f, 0.0f};
        m_player.SetPlayerPosition(safeSpawn);

        m_player.GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});
        m_player.GetPhysics().SetGroundLevel(false);
    }
}

// ---------------- Helper methods ----------------

void Engine::EnsureGroundPlaneExists()
{
    if (!m_collisionManager.GetColliders().empty())
        return;

    Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
    Vector3 groundSize = PhysicsComponent::GROUND_COLLISION_SIZE;

    Collision plane{groundCenter, groundSize};
    plane.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(plane);

    TraceLog(LOG_WARNING,
             "Created emergency ground plane in CheckPlayerBounds with size: {%.1f, %.1f, %.1f} at "
             "position: {%.1f, %.1f, %.1f}",
             groundSize.x, groundSize.y, groundSize.z, groundCenter.x, groundCenter.y,
             groundCenter.z);
}

bool Engine::IsPlayerOutOfBounds(const Vector3 &pos) const
{
    const float MIN_Y = PhysicsComponent::WORLD_FLOOR_Y - 5.0f;
    const float MAX_Y = 1000.0f; // Increased from 30.0f to 100.0f to allow higher jumps
    const float MAX_XZ = 1000.0f;
    TraceLog(LOG_INFO, "Checking player bounds at (%.2f, %.2f, %.2f): Y_limit=%f, XZ_limit=%f",
             pos.x, pos.y, pos.z, MAX_Y, MAX_XZ);

    bool outOfBounds =
        (pos.y < MIN_Y || pos.y > MAX_Y || fabs(pos.x) > MAX_XZ || fabs(pos.z) > MAX_XZ);

    if (outOfBounds)
    {
        TraceLog(LOG_WARNING, "Player OUT OF BOUNDS detected at (%.2f, %.2f, %.2f)", pos.x, pos.y,
                 pos.z);
    }

    return outOfBounds;
}

void Engine::TracePlayerIssue(const Vector3 &pos, const Vector3 &vel) const
{
    if (IsPlayerOutOfBounds(pos))
    {
        TraceLog(LOG_WARNING, "Player out of bounds at (%.1f, %.1f, %.1f), respawning...", pos.x,
                 pos.y, pos.z);
    }

    if (m_physics.HasExtremeVelocity(vel))
    {
        TraceLog(LOG_WARNING, "Player has extreme velocity (%.1f, %.1f, %.1f), respawning...",
                 vel.x, vel.y, vel.z);
    }
}

void Engine::HandleKeyboardShortcuts() const { m_manager.ProcessInput(); }

// ==================== RENDERING METHODS ====================

void Engine::Render()
{
    m_renderManager.BeginFrame();

    if (m_showMenu)
    {
        m_renderManager.RenderMenu(m_menu);
    }
    else
    {
        m_renderManager.RenderGame(m_player, m_models, m_collisionManager, m_showCollisionDebug);
    }

    if (m_showDebug)
    {
        m_renderManager.RenderDebugInfo(m_player, m_models, m_collisionManager);
    }

    m_renderManager.EndFrame();
}

// ==================== TESTING & UTILITY METHODS ====================

void Engine::TestOctreeRayCasting()
{
    TraceLog(LOG_INFO, "=== Testing Octree Ray Casting ===");

    Camera &camera = m_player.GetCameraController()->GetCamera();

    // Test ray from camera forward
    Vector3 rayOrigin = camera.position;
    Vector3 rayDirection = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    CollisionRay testRay(rayOrigin, rayDirection);

    TraceLog(LOG_INFO, "Casting ray from camera position: (%.2f, %.2f, %.2f)", rayOrigin.x,
             rayOrigin.y, rayOrigin.z);
    TraceLog(LOG_INFO, "Ray direction: (%.2f, %.2f, %.2f)", rayDirection.x, rayDirection.y,
             rayDirection.z);

    // Test against all BVH-enabled colliders
    const auto &colliders = m_collisionManager.GetColliders();
    bool anyHit = false;
    float closestDistance = FLT_MAX;
    Vector3 closestHitPoint, closestHitNormal;

    for (size_t i = 0; i < colliders.size(); i++)
    {
        if (colliders[i].IsUsingOctree())
        {
            float distance;
            Vector3 hitPoint, hitNormal;

            if (colliders[i].RaycastOctree(testRay.origin, testRay.direction, 1000.0f, distance,
                                           hitPoint, hitNormal))
            {
                anyHit = true;
                TraceLog(LOG_INFO, "Octree Ray hit collider %zu at distance: %.2f", i, distance);
                TraceLog(LOG_INFO, "  Hit point: (%.2f, %.2f, %.2f)", hitPoint.x, hitPoint.y,
                         hitPoint.z);
                TraceLog(LOG_INFO, "  Hit normal: (%.2f, %.2f, %.2f)", hitNormal.x, hitNormal.y,
                         hitNormal.z);

                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    closestHitPoint = hitPoint;
                    closestHitNormal = hitNormal;
                }
            }
            else
            {
                TraceLog(LOG_INFO, "Octree Ray missed collider %zu (triangles: %zu)", i,
                         colliders[i].GetTriangleCount());
            }
        }
        else
        {
            TraceLog(LOG_INFO, "Collider %zu doesn't use Octree (AABB only)", i);
        }
    }

    if (anyHit)
    {
        TraceLog(LOG_INFO, "=== Closest Hit ===");
        TraceLog(LOG_INFO, "Distance: %.2f", closestDistance);
        TraceLog(LOG_INFO, "Point: (%.2f, %.2f, %.2f)", closestHitPoint.x, closestHitPoint.y,
                 closestHitPoint.z);
        TraceLog(LOG_INFO, "Normal: (%.2f, %.2f, %.2f)", closestHitNormal.x, closestHitNormal.y,
                 closestHitNormal.z);
    }
    else
    {
        TraceLog(LOG_INFO, "No BVH ray hits detected");
    }
}

void Engine::OptimizeModelPerformance()
{
    TraceLog(LOG_INFO, "Optimizing model performance...");

    m_models.CleanupUnusedModels();
    m_models.OptimizeCache();
    m_models.PrintStatistics();
    m_models.PrintCacheInfo();

    TraceLog(LOG_INFO, "Model performance optimization completed");
}
