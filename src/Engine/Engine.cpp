//
// Engine.cpp - Main Engine Implementation
// Created by I#Oleg on 20.07.2025.
//

#include "Engine.h"

// ==================== INCLUDES ====================

// Standard library
#include <stdexcept>

// Raylib & ImGui
#include <Menu/Menu.h>
#include <imgui.h>
#include <raylib.h>
#include <rcamera.h>
#include <rlImGui.h>

// Collision system
#include <Collision/CollisionSystem.h>

// ==================== CONSTRUCTORS & DESTRUCTOR ====================

Engine::Engine(const int screenX, const int screenY)
    : m_screenX(screenX), m_screenY(screenY), m_windowName("Chained Decos"), m_usePlayerModel(true)
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

    TraceLog(LOG_INFO, "Closing window...");
    CloseWindow();

    TraceLog(LOG_INFO, "Engine destructor completed");
}

// ==================== MAIN API ====================

void Engine::Init()
{
    TraceLog(LOG_INFO, "Initializing Engine...");

    // Configure window flags
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);

    // Initialize window
    InitWindow(m_screenX, m_screenY, m_windowName.c_str());
    SetTargetFPS(60); // Local constant

    // Initialize RenderManager
    m_renderManager.Initialize();

    // Initialize models with improved settings
    const std::string modelsJsonPath = PROJECT_ROOT_DIR "/src/models.json";
    TraceLog(LOG_INFO, "Loading models from: %s", modelsJsonPath.c_str());
    m_models.SetCacheEnabled(true);
    m_models.SetMaxCacheSize(50);
    m_models.EnableLOD(true);

    // Load models from JSON file
    try
    {
        m_models.LoadModelsFromJson(modelsJsonPath);
        m_models.PrintStatistics();
        TraceLog(LOG_INFO, "Models loaded successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, " Failed to load models: %s", e.what());
    }

    // Initialize game systems
    TraceLog(LOG_INFO, "Starting collision system initialization...");
    try
    {
        InitCollisions();
        TraceLog(LOG_INFO, " Collision system initialized successfully");
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, " Failed to initialize collision system: %s", e.what());
    }

    TraceLog(LOG_INFO, " Engine initialization complete!");
}

void Engine::Run()
{
    TraceLog(LOG_INFO, "Starting main game loop...");

    while (!WindowShouldClose() && !m_shouldExit)
    {
        Update();
        Render();
    }

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

    Camera &camera = m_player.GetCameraController()->GetCamera();
    int &cameraMode = m_player.GetCameraController()->GetCameraMode();

    // Function keys
    m_manager.RegisterAction(KEY_F11, [this]() { ToggleFullscreen(); });
    m_manager.RegisterAction(KEY_F1,
                             [this]()
                             {
                                 m_showMenu = true;
                                 EnableCursor();
                             });

    // Camera controls
    m_manager.RegisterAction(KEY_ONE,
                             [this, &camera, &cameraMode]()
                             {
                                 cameraMode = CAMERA_FREE;
                                 camera.up = {0, 1, 0};
                                 TraceLog(LOG_INFO, "Camera mode: FREE");
                             });

    // Debug toggles
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
    m_manager.RegisterAction(KEY_F4, [this]() { m_shouldExit = true; });

    m_manager.RegisterAction(
        KEY_F10,
        [this]()
        {
            TraceLog(LOG_INFO, "F10 pressed - forcing collision debug ON for next frame");
            m_renderManager.ForceCollisionDebugNextFrame();
        });

    m_manager.RegisterAction(KEY_F8, [this]() { OptimizeModelPerformance(); });

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

    // BVH Ray casting test (F12)
    m_manager.RegisterAction(KEY_F12, [this]() { TestBVHRayCasting(); });

    TraceLog(LOG_INFO, "Input bindings configured successfully");
}
// TESTING
void Engine::InitCollisions()
{
    TraceLog(LOG_INFO, "Initializing collision system...");

    // Clear existing collisions
    m_collisionManager.ClearColliders();

    try
    {
        // Use constants for ground collision
        Collision groundCollision{PhysicsComponent::GROUND_COLLISION_CENTER,
                                  PhysicsComponent::GROUND_COLLISION_SIZE};
        groundCollision.SetUseBVH(false);
        m_collisionManager.AddCollider(groundCollision);

        // Arena model collision
        TraceLog(LOG_INFO, "Looking for 'arc' model...");

        // Debug: List all available models
        auto availableModels = m_models.GetAvailableModels();
        TraceLog(LOG_INFO, "Available models (%zu):", availableModels.size());
        for (const auto &modelName : availableModels)
        {
            TraceLog(LOG_INFO, "  - %s", modelName.c_str());
        }

        try
        {
            Model &arenaModel = m_models.GetModelByName("arc");
            TraceLog(LOG_INFO, " 'arc' model found successfully");

            // Create simple mesh collision (AABB) for arena geometry
            Collision arenaMeshCollision;
            arenaMeshCollision.CalculateFromModel(&arenaModel);

            Vector3 aabbMin = arenaMeshCollision.GetMin();
            Vector3 aabbMax = arenaMeshCollision.GetMax();
            TraceLog(LOG_INFO, "Arena AABB: min=(%.2f,%.2f,%.2f) max=(%.2f,%.2f,%.2f)", aabbMin.x,
                     aabbMin.y, aabbMin.z, aabbMax.x, aabbMax.y, aabbMax.z);

            // Use simple mesh collision (no BVH)
            arenaMeshCollision.SetUseBVH(true);
            m_collisionManager.AddCollider(arenaMeshCollision);
            TraceLog(LOG_INFO, "Added arena collision (Mesh/AABB) - faster but less precise");

            // Verify mesh collision is being used
            if (!arenaMeshCollision.IsUsingBVH())
            {
                TraceLog(LOG_INFO, "✓ Mesh collision system is ACTIVE for arena");
            }
            else
            {
                TraceLog(LOG_WARNING, "✗ Mesh collision system FAILED - BVH still active");
            }
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Failed to find or process 'arc' model: %s", e.what());
            TraceLog(LOG_WARNING, "  Collision system will only have ground collision");
        }

        TraceLog(LOG_INFO, " Collision system initialized with %zu colliders",
                 m_collisionManager.GetColliders().size());
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Failed to initialize collisions: %s", e.what());
        throw; // Re-throw to handle at higher level
    }
}

// ==================== UPDATE METHODS ====================

void Engine::Update()
{
    // Handle global input
    if (IsKeyPressed(KEY_ESCAPE))
    {
        ToggleMenu();
    }

    HandleKeyboardShortcuts();

    // Update game systems only if not in menu
    if (!m_showMenu)
    {
        UpdatePlayer();
        UpdatePhysics();
        CheckPlayerBounds();
        HandleMousePicking();
    }
}

void Engine::UpdatePlayer()
{
    // Only update player if ImGui doesn't capture input
    if (const ImGuiIO &io = ImGui::GetIO(); !io.WantCaptureMouse)
    {
        m_player.Update();
    }
}

void Engine::UpdatePhysics()
{
    // Apply physics and handle collisions
    m_player.ApplyGravityForPlayer(m_collisionManager);
    m_player.UpdatePlayerBox();

    // Check and resolve collisions
    Vector3 response = {};
    bool isColliding = m_collisionManager.CheckCollision(m_player.GetCollision(), response);

    if (isColliding)
    {
        m_player.Move(response);
    }
}

void Engine::CheckPlayerBounds()
{
    // Reset player if they fall below the world (improved with constants)
    if (m_player.GetPlayerPosition().y < PhysicsComponent::WORLD_FLOOR_Y)
    {
        TraceLog(LOG_WARNING, "Player fell below world bounds (%.1f), respawning...",
                 PhysicsComponent::WORLD_FLOOR_Y);
        m_player.SetPlayerPosition(Player::DEFAULT_SPAWN_POSITION);
    }
}

void Engine::HandleKeyboardShortcuts() const { m_manager.ProcessInput(); }

void Engine::HandleMousePicking()
{
    // Only handle mouse picking if ImGui doesn't want the mouse
    const ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    // Check for mouse click
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        Vector2 mousePos = GetMousePosition();
        Camera &camera = m_player.GetCameraController()->GetCamera();

        // Convert Raylib Ray to CollisionRay
        Ray raylibRay = GetMouseRay(mousePos, camera);
        CollisionRay mouseRay(raylibRay.position, raylibRay.direction);

        // Find closest BVH hit
        const auto &colliders = m_collisionManager.GetColliders();
        float closestDistance = FLT_MAX;
        Vector3 closestHitPoint = {0};
        Vector3 closestHitNormal = {0, 1, 0};
        bool anyHit = false;

        for (size_t i = 0; i < colliders.size(); i++)
        {
            if (colliders[i].IsUsingBVH())
            {
                float distance;
                Vector3 hitPoint, hitNormal;

                if (colliders[i].RaycastBVH(mouseRay, distance, hitPoint, hitNormal))
                {
                    if (distance < closestDistance)
                    {
                        closestDistance = distance;
                        closestHitPoint = hitPoint;
                        closestHitNormal = hitNormal;
                        anyHit = true;
                    }
                }
            }
        }

        if (anyHit)
        {
            TraceLog(LOG_INFO, "Mouse picked point: (%.2f, %.2f, %.2f) at distance %.2f",
                     closestHitPoint.x, closestHitPoint.y, closestHitPoint.z, closestDistance);
        }
    }
}

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

    // Handle menu actions after frame
    switch (m_menu.GetAction())
    {
    case MenuAction::StartGame:
        m_showMenu = false;
        InitInput();
        HideCursor(); // Hide mouse cursor when game starts
        m_menu.ResetAction();
        TraceLog(LOG_INFO, "Game started from menu");
        break;
    case MenuAction::OpenOptions:
        m_menu.ResetAction();
        TraceLog(LOG_INFO, "Options menu requested");
        break;
    case MenuAction::ExitGame:
        m_menu.ResetAction();
        m_shouldExit = true;
        TraceLog(LOG_INFO, "Exit game requested from menu");
        break;
    default:
        break;
    }
}

// ==================== TESTING & UTILITY METHODS ====================

void Engine::TestBVHRayCasting()
{
    TraceLog(LOG_INFO, "=== Testing BVH Ray Casting ===");

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
        if (colliders[i].IsUsingBVH())
        {
            float distance;
            Vector3 hitPoint, hitNormal;

            if (colliders[i].RaycastBVH(testRay, distance, hitPoint, hitNormal))
            {
                anyHit = true;
                TraceLog(LOG_INFO, "BVH Ray hit collider %zu at distance: %.2f", i, distance);
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
                TraceLog(LOG_INFO, "BVH Ray missed collider %zu (triangles: %zu)", i,
                         colliders[i].GetTriangleCount());
            }
        }
        else
        {
            TraceLog(LOG_INFO, "Collider %zu doesn't use BVH (AABB only)", i);
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

    // Test mouse ray casting if in game mode
    if (!m_showMenu)
    {
        TestMouseRayCasting();
    }
}

void Engine::TestMouseRayCasting()
{
    TraceLog(LOG_INFO, "=== Testing Mouse Ray Casting ===");

    Vector2 mousePos = GetMousePosition();
    Camera &camera = m_player.GetCameraController()->GetCamera();

    // Convert Raylib Ray to CollisionRay
    Ray raylibRay = GetMouseRay(mousePos, camera);
    CollisionRay mouseRay(raylibRay.position, raylibRay.direction);

    TraceLog(LOG_INFO, "Mouse position: (%.0f, %.0f)", mousePos.x, mousePos.y);
    TraceLog(LOG_INFO, "Mouse ray origin: (%.2f, %.2f, %.2f)", raylibRay.position.x,
             raylibRay.position.y, raylibRay.position.z);

    // Test against BVH colliders
    const auto &colliders = m_collisionManager.GetColliders();
    for (size_t i = 0; i < colliders.size(); i++)
    {
        if (colliders[i].IsUsingBVH())
        {
            float distance;
            Vector3 hitPoint, hitNormal;

            if (colliders[i].RaycastBVH(mouseRay, distance, hitPoint, hitNormal))
            {
                TraceLog(LOG_INFO, "Mouse ray hit collider %zu at distance: %.2f", i, distance);
                TraceLog(LOG_INFO, "  World hit point: (%.2f, %.2f, %.2f)", hitPoint.x, hitPoint.y,
                         hitPoint.z);
                break; // Only show first hit
            }
        }
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

// ==================== FUTURE FEATURES (UNUSED) ====================

void Engine::LoadAdditionalModels()
{
    TraceLog(LOG_INFO, "Loading additional models dynamically...");

    if (m_models.LoadSingleModel("pickup", "/resources/pickup.glb", true))
    {
        TraceLog(LOG_INFO, "Pickup model loaded successfully");
        SpawnPickups();
    }
}

void Engine::SpawnPickups()
{
    TraceLog(LOG_INFO, "Spawning pickups...");

    // Local spawning constants
    constexpr int PICKUP_COUNT = 8;
    constexpr float PICKUP_Y_POSITION = 0.5f;
    constexpr float PICKUP_SCALE = 0.5f;
    constexpr int PICKUP_SPAWN_RANGE = 10;

    for (int i = 0; i < PICKUP_COUNT; i++)
    {
        ModelInstanceConfig pickupConfig;
        pickupConfig.position = Vector3{
            (float)(GetRandomValue(-PICKUP_SPAWN_RANGE, PICKUP_SPAWN_RANGE)), PICKUP_Y_POSITION,
            (float)(GetRandomValue(-PICKUP_SPAWN_RANGE, PICKUP_SPAWN_RANGE))};
        pickupConfig.scale = PICKUP_SCALE;
        pickupConfig.color = YELLOW;
        pickupConfig.tag = "pickup";
        pickupConfig.spawn = true;

        if (m_models.AddInstanceEx("pickup", pickupConfig))
        {
            TraceLog(LOG_INFO, "Spawned pickup %d", i + 1);
        }
    }
}