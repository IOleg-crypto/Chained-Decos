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
    : m_screenX(screenX), m_screenY(screenY), m_windowName("Chained Decos"), m_showMenu(true),
      m_shouldExit(false), m_windowInitialized(false), m_showDebug(false),
      m_showCollisionDebug(false)
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

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(m_screenX, m_screenY, m_windowName.c_str());
    m_windowInitialized = true;
    SetTargetFPS(60);

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

    m_player.UpdatePlayerBox();
    m_player.UpdatePlayerCollision();

    if (!m_collisionManager.GetColliders().empty())
    {

        Vector3 groundTop = PhysicsComponent::GROUND_COLLISION_CENTER;
        groundTop.y += PhysicsComponent::GROUND_COLLISION_SIZE.y / 2.0f;

        Vector3 safePosition = {0.0f, groundTop.y + 0.01f, 0.0f};
        m_player.SetPlayerPosition(safePosition);

        m_player.UpdatePlayerBox();
        m_player.UpdatePlayerCollision();

        m_player.GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});
        m_player.GetPhysics().SetGroundLevel(true);
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
    CreateAutoCollisionsFromModels();

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

void Engine::CreateAutoCollisionsFromModels()
{
    TraceLog(LOG_INFO, "Starting automatic collision generation for all models...");

    // Get all available models
    auto availableModels = m_models.GetAvailableModels();
    TraceLog(LOG_INFO, "Found %zu models to check", availableModels.size());

    // Track processed models to avoid duplication
    std::set<std::string> processedModels;
    int collisionsCreated = 0;
    constexpr size_t MAX_COLLISION_INSTANCES = 3; // Reduced safety limit for better performance

    // Process each model
    for (const auto &modelName : availableModels)
    {
        // Skip if already processed
        if (processedModels.contains(modelName))
        {
            continue;
        }

        processedModels.insert(modelName);
        TraceLog(LOG_INFO, "Processing model: %s", modelName.c_str());

        try
        {
            // Get the model and check if it should have collision
            Model &model = m_models.GetModelByName(modelName);
            bool hasCollision = m_models.HasCollision(modelName);

            // Skip models without collision or meshes
            if (!hasCollision || model.meshCount == 0)
            {
                TraceLog(LOG_INFO, "Skipping model '%s': hasCollision=%s, meshCount=%d",
                         modelName.c_str(), hasCollision ? "true" : "false", model.meshCount);
                continue;
            }

            // Find instances of this model
            auto instances = m_models.GetInstancesByTag(modelName);

            if (instances.empty())
            {
                // No instances found, create default collision
                Vector3 defaultPos = (modelName == "arc") ? Vector3{0, 0, 140} : Vector3{0, 0, 0};
                if (CreateCollisionFromModel(model, modelName, defaultPos, 1.0f))
                {
                    collisionsCreated++;
                }
            }
            else
            {
                // Create collisions for each instance (up to the limit)
                size_t instanceLimit = std::min(instances.size(), MAX_COLLISION_INSTANCES);
                TraceLog(LOG_INFO, "Processing %zu/%zu instances for model '%s'", instanceLimit,
                         instances.size(), modelName.c_str());

                for (size_t i = 0; i < instanceLimit; i++)
                {
                    auto *instance = instances[i];
                    Vector3 position = instance->GetModelPosition();
                    float scale = instance->GetScale();

                    if (CreateCollisionFromModel(model, modelName, position, scale))
                    {
                        collisionsCreated++;
                    }
                }

                if (instances.size() > MAX_COLLISION_INSTANCES)
                {
                    TraceLog(LOG_WARNING,
                             "Limited collisions for model '%s' to %zu (of %zu instances)",
                             modelName.c_str(), MAX_COLLISION_INSTANCES, instances.size());
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

// Helper function to create cache key
std::string Engine::MakeCollisionCacheKey(const std::string &modelName, float scale) const
{
    // Round scale to 1 decimal place to avoid cache misses for tiny differences
    int scaledInt = static_cast<int>(scale * 10.0f);
    return modelName + "_s" + std::to_string(scaledInt);
}

bool Engine::CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                      Vector3 position, float scale)
{
    TraceLog(LOG_INFO,
             "Creating collision from model '%s' at position (%.2f, %.2f, %.2f) scale=%.2f",
             modelName.c_str(), position.x, position.y, position.z, scale);

    // --- STEP 1: Get model configuration and determine collision type ---
    const ModelFileConfig *config = m_models.GetModelConfig(modelName);
    bool needsPreciseCollision = false;

    if (config)
    {
        // Check if model needs precise collision based on its configuration
        needsPreciseCollision =
            (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE ||
             config->collisionPrecision == CollisionPrecision::OCTREE_ONLY ||
             config->collisionPrecision == CollisionPrecision::IMPROVED_AABB ||
             config->collisionPrecision == CollisionPrecision::AUTO);
    }
    else
    {
        TraceLog(LOG_WARNING, "No config found for model '%s'", modelName.c_str());
    }

    // --- STEP 2: Check collision cache or create new collision ---
    std::string cacheKey = modelName;
    std::shared_ptr<Collision> cachedCollision;

    // Try to find in cache first
    auto cacheIt = m_collisionCache.find(cacheKey);
    if (cacheIt != m_collisionCache.end())
    {
        cachedCollision = cacheIt->second;
        TraceLog(LOG_INFO, "Using cached collision for '%s'", cacheKey.c_str());
    }
    else
    {
        // Need to build a new collision
        cachedCollision = CreateBaseCollision(model, modelName, config, needsPreciseCollision);
        m_collisionCache[cacheKey] = cachedCollision;
    }

    // --- STEP 3: Create instance collision from cached collision ---
    Collision instanceCollision;
    CollisionType cachedType = cachedCollision->GetCollisionType();

    // Determine if we need precise collision for this instance
    bool usePreciseForInstance =
        needsPreciseCollision &&
        (cachedType == CollisionType::OCTREE_ONLY || cachedType == CollisionType::IMPROVED_AABB ||
         cachedType == CollisionType::TRIANGLE_PRECISE);

    if (usePreciseForInstance)
    {
        // Check if we've reached the limit for precise collisions for this model
        int &preciseCount = m_preciseCollisionCount[modelName];

        if (preciseCount < MAX_PRECISE_COLLISIONS_PER_MODEL &&
            strcmp(modelName.c_str(), "arc") == 0) // Only use precise collision for "arc" model
        {
            // Create precise collision with full transformation
            instanceCollision = CreatePreciseInstanceCollision(model, position, scale, config);
            preciseCount++;
        }
        else
        {
            // Fallback to AABB if we reached the limit
            TraceLog(LOG_WARNING,
                     "Reached limit of %d precise collisions for model '%s', using AABB",
                     MAX_PRECISE_COLLISIONS_PER_MODEL, modelName.c_str());
            instanceCollision = CreateSimpleInstanceCollision(*cachedCollision, position, scale);
        }
    }
    else
    {
        // Create simple AABB collision
        instanceCollision = CreateSimpleInstanceCollision(*cachedCollision, position, scale);
    }

    // Add the instance collision to collision manager
    size_t beforeCount = m_collisionManager.GetColliders().size();
    m_collisionManager.AddCollider(instanceCollision);
    size_t afterCount = m_collisionManager.GetColliders().size();

    if (afterCount > beforeCount)
    {
        TraceLog(LOG_INFO,
                 "Successfully created instance collision for '%s', collider count: %zu -> %zu",
                 modelName.c_str(), beforeCount, afterCount);
        return true;
    }
    else
    {
        TraceLog(LOG_ERROR, "FAILED to add collision for '%s', collider count unchanged: %zu",
                 modelName.c_str(), beforeCount);
        return false;
    }
}

// Helper method to create base collision for caching
std::shared_ptr<Collision> Engine::CreateBaseCollision(const Model &model,
                                                       const std::string &modelName,
                                                       const ModelFileConfig *config,
                                                       bool needsPreciseCollision)
{
    std::shared_ptr<Collision> collision;

    // Check if model has valid geometry
    bool hasValidGeometry = false;
    for (int i = 0; i < model.meshCount; i++)
    {
        if (model.meshes[i].vertices && model.meshes[i].vertexCount > 0)
        {
            hasValidGeometry = true;
            break;
        }
    }

    if (!hasValidGeometry)
    {
        // Create fallback AABB collision for models without geometry
        TraceLog(LOG_WARNING, "Model '%s' has no valid geometry, creating fallback collision",
                 modelName.c_str());
        BoundingBox modelBounds = GetModelBoundingBox(model);
        Vector3 size = {modelBounds.max.x - modelBounds.min.x,
                        modelBounds.max.y - modelBounds.min.y,
                        modelBounds.max.z - modelBounds.min.z};
        Vector3 center = {(modelBounds.max.x + modelBounds.min.x) * 0.5f,
                          (modelBounds.max.y + modelBounds.min.y) * 0.5f,
                          (modelBounds.max.z + modelBounds.min.z) * 0.5f};

        collision = std::make_shared<Collision>(center, size);
    }
    else
    {
        // Create collision from model geometry
        collision = std::make_shared<Collision>();
        Model modelCopy = model; // Make a copy for collision building

        // Build base collision at origin without transformation
        if (config)
        {
            collision->BuildFromModelConfig(&modelCopy, *config, MatrixIdentity());
        }
        else
        {
            collision->BuildFromModel(&modelCopy, MatrixIdentity());
        }

        // Set correct collision type for precise configs
        if (needsPreciseCollision && config)
        {
            CollisionType targetType = CollisionType::HYBRID_AUTO;

            if (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE)
            {
                targetType = CollisionType::TRIANGLE_PRECISE;
            }
            else if (config->collisionPrecision == CollisionPrecision::OCTREE_ONLY)
            {
                targetType = CollisionType::OCTREE_ONLY;
            }
            else if (config->collisionPrecision == CollisionPrecision::IMPROVED_AABB)
            {
                targetType = CollisionType::IMPROVED_AABB;
            }

            collision->SetCollisionType(targetType);
        }
    }

    return collision;
}

// Helper method to create precise instance collision
Collision Engine::CreatePreciseInstanceCollision(const Model &model, Vector3 position, float scale,
                                                 const ModelFileConfig *config)
{
    Collision instanceCollision;

    // Create transformation matrix with both scale and position
    Matrix transform = MatrixMultiply(MatrixScale(scale, scale, scale),
                                      MatrixTranslate(position.x, position.y, position.z));

    // Create a copy of the model for collision building
    Model modelCopy = model;

    // Build collision with full transformation
    if (config)
    {
        instanceCollision.BuildFromModelConfig(&modelCopy, *config, transform);
    }
    else
    {
        instanceCollision.BuildFromModel(&modelCopy, transform);
    }

    // Use OCTREE_ONLY for models (more stable than TRIANGLE_PRECISE)
    instanceCollision.SetCollisionType(CollisionType::OCTREE_ONLY);

    TraceLog(LOG_INFO, "Built OCTREE collision for instance at (%.2f, %.2f, %.2f)", position.x,
             position.y, position.z);

    return instanceCollision;
}

// Helper method to create simple AABB instance collision
Collision Engine::CreateSimpleInstanceCollision(const Collision &cachedCollision, Vector3 position,
                                                float scale)
{
    Collision instanceCollision = cachedCollision;

    // Apply transformation to center and scale
    Vector3 cachedCenter = cachedCollision.GetCenter();
    Vector3 cachedSize = cachedCollision.GetSize();
    Vector3 transformedCenter = Vector3Add(Vector3Scale(cachedCenter, scale), position);
    Vector3 scaledSize = Vector3Scale(cachedSize, scale);

    instanceCollision.Update(transformedCenter, scaledSize);

    TraceLog(LOG_INFO, "Created AABB collision for instance at (%.2f, %.2f, %.2f)", position.x,
             position.y, position.z);

    return instanceCollision;
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
        m_player.GetPhysics().SetVelocity(Vector3Zero());
    }
}

void Engine::UpdatePhysics()
{
    // Verify collision system has colliders
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

    // Apply gravity and handle collisions
    m_player.ApplyGravityForPlayer(m_collisionManager);
    m_player.UpdatePlayerBox();
}

void Engine::CheckPlayerBounds()
{
    EnsureGroundPlaneExists();

    Vector3 playerPos = m_player.GetPlayerPosition();
    Vector3 playerVel = m_player.GetPhysics().GetVelocity();

    if (IsPlayerOutOfBounds(playerPos) || HasExtremeVelocity(playerVel))
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

bool Engine::HasExtremeVelocity(const Vector3 &vel) const
{
    const float MAX_SPEED = 300.0f; // Increased from 90.0f to 150.0f to allow higher jumps
    return Vector3Length(vel) > MAX_SPEED;
}

void Engine::TracePlayerIssue(const Vector3 &pos, const Vector3 &vel) const
{
    if (IsPlayerOutOfBounds(pos))
    {
        TraceLog(LOG_WARNING, "Player out of bounds at (%.1f, %.1f, %.1f), respawning...", pos.x,
                 pos.y, pos.z);
    }

    if (HasExtremeVelocity(vel))
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
    // Handle menu actions after frame
    Vector3 safePosition = {0.0f, 2.0f, 0.0f}; // Lower starting position (2.0 instead of 5.0)
    switch (m_menu.GetAction())
    {
    case MenuAction::SinglePlayer:
        m_showMenu = false;

        // Reinitialize collision system when starting game from menu
        TraceLog(LOG_INFO, "Reinitializing collision system for game start...");
        InitCollisions();

        // Reinitialize input
        InitInput();
        HideCursor(); // Hide mouse cursor when game starts

        // Reset player position to a safe starting point

        // Reset player physics state
        m_player.SetPlayerPosition(safePosition);
        m_player.GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f}); // Ensure zero velocity
        m_player.GetPhysics().SetGroundLevel(true);            // Start in air
        m_player.UpdatePlayerBox();
        m_player.UpdatePlayerCollision();

        // Apply initial gravity to ensure player is grounded
        if (!m_collisionManager.GetColliders().empty())
        {
            TraceLog(LOG_INFO, "Applying initial gravity with %zu colliders",
                     m_collisionManager.GetColliders().size());

            m_player.ApplyGravityForPlayer(m_collisionManager);
            Vector3 pos = m_player.GetPlayerPosition();
            TraceLog(LOG_INFO, "Applied initial gravity, player position: (%.2f, %.2f, %.2f)",
                     pos.x, pos.y, pos.z);
        }
        else
        {
            TraceLog(LOG_ERROR, "Cannot apply gravity - no colliders available!");
            // Create emergency ground plane
            Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
            Vector3 groundSize = PhysicsComponent::GROUND_COLLISION_SIZE;
            Collision plane{groundCenter, groundSize};
            plane.SetCollisionType(CollisionType::AABB_ONLY);
            m_collisionManager.AddCollider(plane);
            TraceLog(LOG_WARNING,
                     "Created emergency ground plane in StartGame with size: {%.1f, %.1f, %.1f} at "
                     "position: {%.1f, %.1f, %.1f}",
                     groundSize.x, groundSize.y, groundSize.z, groundCenter.x, groundCenter.y,
                     groundCenter.z);
        }

        m_menu.ResetAction();
        TraceLog(LOG_INFO, "Game started from menu with %zu colliders",
                 m_collisionManager.GetColliders().size());
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