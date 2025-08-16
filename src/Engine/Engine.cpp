//
// Engine.cpp - Main Engine Implementation
// Created by I#Oleg on 20.07.2025.
//

#include "Engine.h"

// Standard library
#include <stdexcept>

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
    : m_screenX(screenX), m_screenY(screenY), m_windowName("Chained Decos"), m_shouldExit(false),
      m_showMenu(true), m_windowInitialized(false), m_showDebug(false), m_showCollisionDebug(false)
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

    // Configure window flags
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);

    // Initialize window
    InitWindow(m_screenX, m_screenY, m_windowName.c_str());
    m_windowInitialized = true; // Mark that we initialized the window
    SetTargetFPS(60);           // Local constant

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

    // Initialize input system
    TraceLog(LOG_INFO, "Initializing input system...");
    InitInput();
    TraceLog(LOG_INFO, " Input system initialized successfully");

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

    TraceLog(LOG_INFO, "Input bindings configured successfully");
}
// TESTING
void Engine::InitCollisions()
{
    TraceLog(LOG_INFO, "Initializing collision system...");

    // Automatically create collisions for all models with hasCollision=true
    CreateAutoCollisionsFromModels();

    // Create a ground plane LAST (lower priority) - solid surface at y=0
    Collision plane{(Vector3){0, -5, 0}, (Vector3){1000, 10, 1000}};
    // Simple plane uses AABB by default (no need to set anything)
    m_collisionManager.AddCollider(plane);
    TraceLog(LOG_INFO,
             "Ground plane added to Legacy system: center(0,-5,0) extends from y=-10 to y=0");
}

void Engine::CreateAutoCollisionsFromModels()
{
    TraceLog(LOG_INFO, "Starting automatic collision generation for all models...");

    // Get all available models
    auto availableModels = m_models.GetAvailableModels();
    TraceLog(LOG_INFO, "Found %zu models to check", availableModels.size());

    int collisionsCreated = 0;

    // Process each model
    for (const auto &modelName : availableModels)
    {
        TraceLog(LOG_INFO, "Processing model: %s", modelName.c_str());

        try
        {
            // Get the model
            Model &model = m_models.GetModelByName(modelName);

            if (model.meshCount == 0)
            {
                TraceLog(LOG_WARNING, "Model '%s' has no meshes, skipping collision creation",
                         modelName.c_str());
                continue;
            }

            TraceLog(LOG_INFO, "Model '%s' has %d meshes", modelName.c_str(), model.meshCount);

            // Find instances of this model
            auto instances = m_models.GetInstancesByTag(modelName);

            if (instances.empty())
            {
                TraceLog(LOG_INFO,
                         "No instances found for model '%s', trying with default position",
                         modelName.c_str());

                // Check if this model should have collision based on JSON config
                if (m_models.HasCollision(modelName))
                {
                    Vector3 defaultPos =
                        (modelName == "arc") ? Vector3{0, 0, 140} : Vector3{0, 0, 0};
                    if (CreateCollisionFromModel(model, modelName, defaultPos, 1.0f))
                    {
                        collisionsCreated++;
                        TraceLog(LOG_INFO, "Created default collision for model '%s'",
                                 modelName.c_str());
                    }
                }
            }
            else
            {
                // Check if this model should have collision based on JSON config
                if (m_models.HasCollision(modelName))
                {
                    // Create collisions for each instance
                    for (auto *instance : instances)
                    {
                        Vector3 position = instance->GetModelPosition();
                        float scale = instance->GetScale();

                        TraceLog(
                            LOG_INFO,
                            "Creating collision for '%s' instance at (%.1f, %.1f, %.1f) scale=%.2f",
                            modelName.c_str(), position.x, position.y, position.z, scale);

                        if (CreateCollisionFromModel(model, modelName, position, scale))
                        {
                            collisionsCreated++;
                            TraceLog(LOG_INFO, "Successfully created collision for '%s' instance",
                                     modelName.c_str());
                        }
                    }
                }
                else
                {
                    TraceLog(LOG_INFO,
                             "Model '%s' has hasCollision=false, skipping collision creation",
                             modelName.c_str());
                }
            }
        }
        catch (const std::exception &e)
        {
            TraceLog(LOG_ERROR, "Failed to create collision for model '%s': %s", modelName.c_str(),
                     e.what());
        }
    }

    TraceLog(LOG_INFO,
             "Automatic collision generation complete. Created %d collisions from %zu models",
             collisionsCreated, availableModels.size());
}

bool Engine::CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                      Vector3 position, float scale)
{
    TraceLog(LOG_INFO, "Creating precise collision from model with %d meshes", model.meshCount);

    // Create transformation matrix
    Matrix transform = MatrixMultiply(MatrixScale(scale, scale, scale),
                                      MatrixTranslate(position.x, position.y, position.z));

    // Check if we have valid geometry
    bool hasValidGeometry = false;
    for (int i = 0; i < model.meshCount; i++)
    {
        if (model.meshes[i].vertices && model.meshes[i].vertexCount > 0)
        {
            hasValidGeometry = true;
            TraceLog(LOG_INFO, "Mesh %d: %d vertices, %d triangles", i, model.meshes[i].vertexCount,
                     model.meshes[i].triangleCount);
        }
    }

    if (!hasValidGeometry)
    {
        TraceLog(LOG_WARNING, "Model has no valid geometry, creating fallback collision");
        // Fallback: create simple AABB collision
        BoundingBox modelBounds = GetModelBoundingBox(model);
        Vector3 size = {(modelBounds.max.x - modelBounds.min.x) * scale,
                        (modelBounds.max.y - modelBounds.min.y) * scale,
                        (modelBounds.max.z - modelBounds.min.z) * scale};
        Vector3 center = {position.x + (modelBounds.max.x + modelBounds.min.x) * 0.5f * scale,
                          position.y + (modelBounds.max.y + modelBounds.min.y) * 0.5f * scale,
                          position.z + (modelBounds.max.z + modelBounds.min.z) * 0.5f * scale};

        Collision fallbackCollision{center, size};
        // Fallback collision is simple AABB - no need to change anything
        m_collisionManager.AddCollider(fallbackCollision);
        return true;
    }

    // Create collision object with hybrid system (automatically chooses optimal method)
    Collision modelCollision;

    // Create a non-const copy for processing
    Model modelCopy = model;

    TraceLog(LOG_INFO, "Building hybrid collision from model geometry...");

    // Get model configuration to determine collision precision
    const ModelFileConfig *config = m_models.GetModelConfig(modelName);

    if (config)
    {
        TraceLog(LOG_INFO, "Using model config for collision precision: %s (hasCollision=%s)",
                 (config->collisionPrecision == CollisionPrecision::AABB_ONLY)       ? "AABB"
                 : (config->collisionPrecision == CollisionPrecision::IMPROVED_AABB) ? "IMPROVED"
                                                                                     : "PRECISE",
                 config->hasCollision ? "true" : "false");

        // Build collision using model configuration
        modelCollision.BuildFromModelConfig(&modelCopy, *config, transform);
    }
    else
    {
        TraceLog(LOG_WARNING, "No config found for model '%s', using hybrid auto-detection",
                 modelName.c_str());
        // Fallback: Build collision using hybrid system (automatically chooses AABB or Octree based
        // on complexity)
        modelCollision.BuildFromModel(&modelCopy, transform);
    }

    // Log collision details
    Vector3 collisionMin = modelCollision.GetMin();
    Vector3 collisionMax = modelCollision.GetMax();
    Vector3 collisionCenter = modelCollision.GetCenter();
    Vector3 collisionSize = modelCollision.GetSize();

    // Get complexity analysis
    const auto &complexity = modelCollision.GetComplexity();
    CollisionType collisionType = modelCollision.GetCollisionType();

    TraceLog(LOG_INFO, "Collision created:");
    TraceLog(LOG_INFO, "  Type: %s",
             (collisionType == CollisionType::AABB_ONLY)          ? "AABB_ONLY"
             : (collisionType == CollisionType::IMPROVED_AABB)    ? "IMPROVED_AABB"
             : (collisionType == CollisionType::TRIANGLE_PRECISE) ? "TRIANGLE_PRECISE"
             : (collisionType == CollisionType::OCTREE_ONLY)      ? "OCTREE_ONLY"
                                                                  : "HYBRID_AUTO");
    TraceLog(LOG_INFO, "  Min: (%.2f, %.2f, %.2f)", collisionMin.x, collisionMin.y, collisionMin.z);
    TraceLog(LOG_INFO, "  Max: (%.2f, %.2f, %.2f)", collisionMax.x, collisionMax.y, collisionMax.z);
    TraceLog(LOG_INFO, "  Center: (%.2f, %.2f, %.2f)", collisionCenter.x, collisionCenter.y,
             collisionCenter.z);
    TraceLog(LOG_INFO, "  Size: (%.2f, %.2f, %.2f)", collisionSize.x, collisionSize.y,
             collisionSize.z);
    TraceLog(LOG_INFO, "  Triangle count: %zu", modelCollision.GetTriangleCount());
    TraceLog(LOG_INFO, "  Model complexity: %s", complexity.IsSimple() ? "SIMPLE" : "COMPLEX");

    if (collisionType == CollisionType::OCTREE_ONLY ||
        collisionType == CollisionType::IMPROVED_AABB ||
        collisionType == CollisionType::TRIANGLE_PRECISE)
    {
        TraceLog(LOG_INFO, "  Octree nodes: %zu", modelCollision.GetNodeCount());
    }

    // Add to legacy collision manager
    m_collisionManager.AddCollider(modelCollision);

    // Also create smart collision for improved performance
    CreateCollisionFromModel(model, modelName, position, scale);

    TraceLog(LOG_INFO, "Successfully created hybrid collision from model geometry");
    return true;
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

        // Check collisions immediately after player movement
        m_player.UpdatePlayerBox(); // Ensure collision box is updated
        Vector3 response = {};
        bool isColliding = m_collisionManager.CheckCollision(m_player.GetCollision(), response);

        if (isColliding)
        {
            // TraceLog(LOG_INFO,
            //          "🏃 Immediate collision detected, applying response: (%.2f,%.2f,%.2f)",
            //          response.x, response.y, response.z);
            m_player.Move(response);
        }
    }
    m_renderManager.ShowMetersPlayer(m_player);
}

void Engine::UpdatePhysics()
{
    // Use legacy collision system
    // TraceLog(LOG_INFO, "Using Legacy Collision System (legacy colliders: %zu)",
    //          m_collisionManager.GetColliders().size());
    m_player.ApplyGravityForPlayer(m_collisionManager);

    m_player.UpdatePlayerBox();
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
            // Try raycast with hybrid system (uses Octree if available)
            if (colliders[i].IsUsingOctree())
            {
                float distance;
                Vector3 hitPoint, hitNormal;

                if (colliders[i].RaycastOctree(mouseRay.origin, mouseRay.direction, 1000.0f,
                                               distance, hitPoint, hitNormal))
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

// ==================== FUTURE FEATURES (UNUSED) ====================

// void Engine::LoadAdditionalModels()
// {
//     TraceLog(LOG_INFO, "Loading additional models dynamically...");

//     if (m_models.LoadSingleModel("pickup", "/resources/pickup.glb", true))
//     {
//         TraceLog(LOG_INFO, "Pickup model loaded successfully");
//         SpawnPickups();
//     }
// }

// void Engine::SpawnPickups()
// {
//     TraceLog(LOG_INFO, "Spawning pickups...");

//     // Local spawning constants
//     constexpr int PICKUP_COUNT = 8;
//     constexpr float PICKUP_Y_POSITION = 0.5f;
//     constexpr float PICKUP_SCALE = 0.5f;
//     constexpr int PICKUP_SPAWN_RANGE = 10;

//     for (int i = 0; i < PICKUP_COUNT; i++)
//     {
//         ModelInstanceConfig pickupConfig;
//         pickupConfig.position = Vector3{
//             (float)(GetRandomValue(-PICKUP_SPAWN_RANGE, PICKUP_SPAWN_RANGE)), PICKUP_Y_POSITION,
//             (float)(GetRandomValue(-PICKUP_SPAWN_RANGE, PICKUP_SPAWN_RANGE))};
//         pickupConfig.scale = PICKUP_SCALE;
//         pickupConfig.color = YELLOW;
//         pickupConfig.tag = "pickup";
//         pickupConfig.spawn = true;

//         if (m_models.AddInstanceEx("pickup", pickupConfig))
//         {
//             TraceLog(LOG_INFO, "Spawned pickup %d", i + 1);
//         }
//     }
// }