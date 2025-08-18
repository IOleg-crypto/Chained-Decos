//
// Engine.cpp - Main Engine Implementation
// Created by I#Oleg on 20.07.2025.
//

#include "Engine.h"

// Standard library
#include <set>
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

    // // Camera controls
    // m_manager.RegisterAction(KEY_ONE,
    //                          [this, &camera, &cameraMode]()
    //                          {
    //                              cameraMode = CAMERA_FREE;
    //                              camera.up = {0, 1, 0};
    //                              TraceLog(LOG_INFO, "Camera mode: FREE");
    //                          });

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
// TESTING(add plane)
void Engine::InitCollisions()
{
    TraceLog(LOG_INFO, "🔥 InitCollisions: Starting collision system initialization...");

    // Automatically create collisions for all models with hasCollision=true
    TraceLog(LOG_INFO, "🔥 InitCollisions: About to call CreateAutoCollisionsFromModels()...");
    CreateAutoCollisionsFromModels();
    TraceLog(LOG_INFO, "🔥 InitCollisions: CreateAutoCollisionsFromModels() completed");

    // Create a ground plane LAST (lower priority) - solid surface matching physics constants
    // Use reasonable ground plane size instead of massive 1000x1000
    Vector3 groundCenter = PhysicsComponent::GROUND_COLLISION_CENTER;
    Vector3 groundSize = {1000.0f, 5.0f, 1000.0f}; // Much smaller, reasonable ground plane
    
    Collision plane{groundCenter, groundSize};
    // Explicitly set ground plane to use AABB_ONLY (simple plane collision)
    plane.SetCollisionType(CollisionType::AABB_ONLY);
    m_collisionManager.AddCollider(plane);
    TraceLog(LOG_INFO,
             "Ground plane added to Legacy system: center(%.1f,%.1f,%.1f) size(%.1f,%.1f,%.1f)",
             groundCenter.x, groundCenter.y, groundCenter.z,
             groundSize.x, groundSize.y, groundSize.z);
}

void Engine::CreateAutoCollisionsFromModels()
{
    TraceLog(LOG_INFO, "🚀 AUTO: Starting automatic collision generation for all models...");

    // Get all available models
    auto availableModels = m_models.GetAvailableModels();
    TraceLog(LOG_INFO, "🚀 AUTO: Found %zu models to check", availableModels.size());

    // Track processed models to avoid duplication
    std::set<std::string> processedModels;
    int collisionsCreated = 0;

    // Process each model
    for (const auto &modelName : availableModels)
    {
        // Skip if already processed
        if (processedModels.find(modelName) != processedModels.end())
        {
            TraceLog(LOG_INFO, "Model '%s' already processed, skipping...", modelName.c_str());
            continue;
        }

        processedModels.insert(modelName);
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
            TraceLog(LOG_INFO, "🔎 Model '%s': Found %zu instances by tag", modelName.c_str(),
                     instances.size());

            if (instances.empty())
            {
                TraceLog(LOG_INFO,
                         "No instances found for model '%s', trying with default position",
                         modelName.c_str());

                // Check if this model should have collision based on JSON config
                bool hasCollision = m_models.HasCollision(modelName);
                TraceLog(LOG_INFO, "🔧 Model '%s': HasCollision=%s", modelName.c_str(),
                         hasCollision ? "TRUE" : "FALSE");

                if (hasCollision)
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
                bool hasCollision = m_models.HasCollision(modelName);
                TraceLog(LOG_INFO, "🔧 Model '%s': HasCollision=%s", modelName.c_str(),
                         hasCollision ? "TRUE" : "FALSE");

                if (hasCollision)
                {
                    // Safety check: limit number of collision instances per model
                    const size_t MAX_COLLISION_INSTANCES = 10;
                    if (instances.size() > MAX_COLLISION_INSTANCES)
                    {
                        TraceLog(LOG_WARNING,
                                 "Model '%s' has %zu instances, limiting to %zu for performance",
                                 modelName.c_str(), instances.size(), MAX_COLLISION_INSTANCES);
                    }

                    // Create collisions for each instance
                    TraceLog(LOG_INFO, "📍 Processing %zu instances for model '%s'",
                             instances.size(), modelName.c_str());
                    size_t processedInstances = 0;
                    for (auto *instance : instances)
                    {
                        if (processedInstances >= MAX_COLLISION_INSTANCES)
                        {
                            TraceLog(
                                LOG_WARNING,
                                "Skipping remaining instances of '%s' to prevent memory issues",
                                modelName.c_str());
                            break;
                        }

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
                        processedInstances++;
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

// Helper function to create cache key
std::string Engine::MakeCollisionCacheKey(const std::string &modelName, float scale) const
{
    // Round scale to 2 decimal places to avoid cache misses for tiny differences
    int scaledInt = static_cast<int>(scale * 100.0f);
    return modelName + "_s" + std::to_string(scaledInt);
}

bool Engine::CreateCollisionFromModel(const Model &model, const std::string &modelName,
                                      Vector3 position, float scale)
{
    TraceLog(
        LOG_INFO,
        "🔧 ENGINE: Creating collision from model '%s' at position (%.2f, %.2f, %.2f) scale=%.2f",
        modelName.c_str(), position.x, position.y, position.z, scale);

    // Get model config to check collision type
    const ModelFileConfig *config = m_models.GetModelConfig(modelName);
    bool needsPreciseCollision = false;

    if (config)
    {
        needsPreciseCollision =
            (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE ||
             config->collisionPrecision == CollisionPrecision::OCTREE_ONLY ||
             config->collisionPrecision == CollisionPrecision::IMPROVED_AABB ||
             config->collisionPrecision == CollisionPrecision::AUTO);

        TraceLog(LOG_INFO, "🎯 CONFIG: Model '%s' precision=%s, hasCollision=%s, needsPrecise=%s",
                 modelName.c_str(),
                 (config->collisionPrecision == CollisionPrecision::AUTO)               ? "AUTO"
                 : (config->collisionPrecision == CollisionPrecision::AABB_ONLY)        ? "AABB"
                 : (config->collisionPrecision == CollisionPrecision::OCTREE_ONLY)      ? "OCTREE"
                 : (config->collisionPrecision == CollisionPrecision::IMPROVED_AABB)    ? "IMPROVED"
                 : (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE) ? "TRIANGLE"
                                                                                        : "UNKNOWN",
                 config->hasCollision ? "true" : "false", needsPreciseCollision ? "YES" : "NO");
    }
    else
    {
        TraceLog(LOG_WARNING, "⚠️ CONFIG: No config found for model '%s'", modelName.c_str());
    }

    // Use simple model name as cache key - we'll handle precise collisions differently
    std::string cacheKey = modelName;

    // Check if we already have a collision for this model+scale combination
    std::shared_ptr<Collision> cachedCollision;
    auto cacheIt = m_collisionCache.find(cacheKey);

    if (cacheIt != m_collisionCache.end())
    {
        cachedCollision = cacheIt->second;
        TraceLog(LOG_INFO, "Using cached collision for '%s'", cacheKey.c_str());
    }
    else
    {
        TraceLog(LOG_INFO, "Building new collision for '%s' with %d meshes", cacheKey.c_str(),
                 model.meshCount);

        // Check if we have valid geometry
        bool hasValidGeometry = false;
        for (int i = 0; i < model.meshCount; i++)
        {
            if (model.meshes[i].vertices && model.meshes[i].vertexCount > 0)
            {
                hasValidGeometry = true;
                TraceLog(LOG_INFO, "Mesh %d: %d vertices, %d triangles", i,
                         model.meshes[i].vertexCount, model.meshes[i].triangleCount);
            }
        }

        if (!hasValidGeometry)
        {
            TraceLog(LOG_WARNING, "Model '%s' has no valid geometry, creating fallback collision",
                     modelName.c_str());
            // Create fallback AABB collision
            BoundingBox modelBounds = GetModelBoundingBox(model);
            Vector3 size = {(modelBounds.max.x - modelBounds.min.x) * scale,
                            (modelBounds.max.y - modelBounds.min.y) * scale,
                            (modelBounds.max.z - modelBounds.min.z) * scale};
            Vector3 center = {(modelBounds.max.x + modelBounds.min.x) * 0.5f * scale,
                              (modelBounds.max.y + modelBounds.min.y) * 0.5f * scale,
                              (modelBounds.max.z + modelBounds.min.z) * 0.5f * scale};

            cachedCollision = std::make_shared<Collision>(center, size);
            m_collisionCache[cacheKey] = cachedCollision;
        }
        else
        {
            // Create collision object with proper scale transformation
            cachedCollision = std::make_shared<Collision>();
            Model modelCopy = model; // Make a copy for collision building

            TraceLog(LOG_INFO, "Building scaled collision from model geometry for '%s'...",
                     cacheKey.c_str());

            // Always build base collision at origin without transformation
            if (config)
            {
                TraceLog(
                    LOG_INFO, "Using model config for collision precision: %s (hasCollision=%s)",
                    (config->collisionPrecision == CollisionPrecision::AUTO)            ? "AUTO"
                    : (config->collisionPrecision == CollisionPrecision::AABB_ONLY)     ? "AABB"
                    : (config->collisionPrecision == CollisionPrecision::IMPROVED_AABB) ? "IMPROVED"
                                                                                        : "PRECISE",
                    config->hasCollision ? "true" : "false");

                // Build base collision without transformation - will be transformed per instance
                cachedCollision->BuildFromModelConfig(&modelCopy, *config, MatrixIdentity());
            }
            else
            {
                TraceLog(LOG_WARNING, "No config found for model '%s', using hybrid auto-detection",
                         modelName.c_str());
                // Build base collision without transformation
                cachedCollision->BuildFromModel(&modelCopy, MatrixIdentity());
            }

            // Cache the collision
            m_collisionCache[cacheKey] = cachedCollision;

            // 🚨 CRITICAL FIX: Ensure cached collision has correct type for precise configs
            if (needsPreciseCollision && config)
            {
                CollisionType targetType;
                const char* targetTypeName;
                
                if (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE) {
                    targetType = CollisionType::TRIANGLE_PRECISE;
                    targetTypeName = "TRIANGLE_PRECISE";
                } else if (config->collisionPrecision == CollisionPrecision::OCTREE_ONLY) {
                    targetType = CollisionType::OCTREE_ONLY;
                    targetTypeName = "OCTREE_ONLY";
                } else if (config->collisionPrecision == CollisionPrecision::IMPROVED_AABB) {
                    targetType = CollisionType::IMPROVED_AABB;
                    targetTypeName = "IMPROVED_AABB";
                } else {
                    targetType = CollisionType::HYBRID_AUTO;
                    targetTypeName = "HYBRID_AUTO";
                }
                
                TraceLog(LOG_INFO, "🔥 FORCING cached collision type to %s for '%s'",
                         targetTypeName, modelName.c_str());
                CollisionType beforeType = cachedCollision->GetCollisionType();
                cachedCollision->SetCollisionType(targetType);
                CollisionType afterType = cachedCollision->GetCollisionType();
                TraceLog(LOG_INFO, "🔧 FORCED cached collision: BEFORE=%s, AFTER=%s",
                         beforeType == CollisionType::TRIANGLE_PRECISE ? "TRIANGLE_PRECISE"
                         : beforeType == CollisionType::OCTREE_ONLY ? "OCTREE_ONLY"
                         : beforeType == CollisionType::IMPROVED_AABB  ? "IMPROVED_AABB"
                                                                       : "OTHER",
                         afterType == CollisionType::TRIANGLE_PRECISE ? "TRIANGLE_PRECISE"
                         : afterType == CollisionType::OCTREE_ONLY ? "OCTREE_ONLY"
                         : afterType == CollisionType::IMPROVED_AABB  ? "IMPROVED_AABB"
                                                                      : "OTHER");
            }
            else
            {
                TraceLog(LOG_ERROR,
                         "🚨 NOT FORCING cached collision type! needsPrecise=%s, config=%s, "
                         "precision=%s",
                         needsPreciseCollision ? "YES" : "NO", config ? "EXISTS" : "NULL",
                         config
                             ? (config->collisionPrecision == CollisionPrecision::TRIANGLE_PRECISE
                                    ? "TRIANGLE"
                                    : "OTHER")
                             : "N/A");
            }

            // Log collision details
            Vector3 cacheMin = cachedCollision->GetMin();
            Vector3 cacheMax = cachedCollision->GetMax();
            const auto &complexity = cachedCollision->GetComplexity();
            CollisionType collisionType = cachedCollision->GetCollisionType();

            TraceLog(LOG_INFO, "Cached collision created for '%s':", cacheKey.c_str());
            TraceLog(LOG_INFO, "  Type: %s",
                     (collisionType == CollisionType::AABB_ONLY)          ? "AABB_ONLY"
                     : (collisionType == CollisionType::IMPROVED_AABB)    ? "IMPROVED_AABB"
                     : (collisionType == CollisionType::TRIANGLE_PRECISE) ? "TRIANGLE_PRECISE"
                     : (collisionType == CollisionType::OCTREE_ONLY)      ? "OCTREE_ONLY"
                                                                          : "HYBRID_AUTO");
            TraceLog(LOG_INFO, "  Cache Min: (%.2f, %.2f, %.2f)", cacheMin.x, cacheMin.y,
                     cacheMin.z);
            TraceLog(LOG_INFO, "  Cache Max: (%.2f, %.2f, %.2f)", cacheMax.x, cacheMax.y,
                     cacheMax.z);
            TraceLog(LOG_INFO, "  Triangle count: %zu", cachedCollision->GetTriangleCount());
            TraceLog(LOG_INFO, "  Model complexity: %s",
                     complexity.IsSimple() ? "SIMPLE" : "COMPLEX");

            if (collisionType == CollisionType::OCTREE_ONLY ||
                collisionType == CollisionType::IMPROVED_AABB ||
                collisionType == CollisionType::TRIANGLE_PRECISE)
            {
                TraceLog(LOG_INFO, "  Octree nodes: %zu", cachedCollision->GetNodeCount());
            }
        }
    }

    // Create instance collision from cached collision
    Collision instanceCollision;
    CollisionType cachedType = cachedCollision->GetCollisionType();

    TraceLog(LOG_INFO, "🎲 INSTANCE: Model '%s' cachedType=%s, needsPrecise=%s", modelName.c_str(),
             (cachedType == CollisionType::AABB_ONLY)          ? "AABB_ONLY"
             : (cachedType == CollisionType::IMPROVED_AABB)    ? "IMPROVED_AABB"
             : (cachedType == CollisionType::TRIANGLE_PRECISE) ? "TRIANGLE_PRECISE"
             : (cachedType == CollisionType::OCTREE_ONLY)      ? "OCTREE_ONLY"
                                                               : "HYBRID_AUTO",
             needsPreciseCollision ? "YES" : "NO");

    if (needsPreciseCollision &&
        (cachedType == CollisionType::OCTREE_ONLY || cachedType == CollisionType::IMPROVED_AABB ||
         cachedType == CollisionType::TRIANGLE_PRECISE))
    {
        // Check if we've reached the limit for precise collisions for this model
        int &preciseCount = m_preciseCollisionCount[modelName];

        if (preciseCount < MAX_PRECISE_COLLISIONS_PER_MODEL)
        {
            // For precise collisions, we need to rebuild with full transformation
            // because Update() only changes AABB but not the octree
            TraceLog(LOG_INFO,
                     "Building precise instance collision %d/%d with full transformation for '%s'",
                     preciseCount + 1, MAX_PRECISE_COLLISIONS_PER_MODEL, modelName.c_str());

            // Create transformation matrix with both scale and position
            Matrix transform = MatrixMultiply(MatrixScale(scale, scale, scale),
                                              MatrixTranslate(position.x, position.y, position.z));

            // Create a copy of the model for collision building
            Model modelCopy = model;

            if (config)
            {
                // Build collision with full transformation applied
                instanceCollision.BuildFromModelConfig(&modelCopy, *config, transform);
            }
            else
            {
                // Fallback: rebuild from model with transformation
                instanceCollision.BuildFromModel(&modelCopy, transform);
            }

            preciseCount++; // Increment counter

            // 🚨 Use OCTREE_ONLY for models (more stable than TRIANGLE_PRECISE)
            CollisionType beforeInstanceType = instanceCollision.GetCollisionType();
            instanceCollision.SetCollisionType(CollisionType::OCTREE_ONLY);
            CollisionType afterInstanceType = instanceCollision.GetCollisionType();
            TraceLog(LOG_ERROR, "🔧 FORCED instance collision: BEFORE=%s, AFTER=%s for '%s'",
                     beforeInstanceType == CollisionType::TRIANGLE_PRECISE ? "TRIANGLE_PRECISE"
                     : beforeInstanceType == CollisionType::IMPROVED_AABB  ? "IMPROVED_AABB"
                     : beforeInstanceType == CollisionType::AABB_ONLY      ? "AABB_ONLY"
                     : beforeInstanceType == CollisionType::OCTREE_ONLY    ? "OCTREE_ONLY"
                                                                           : "OTHER",
                     afterInstanceType == CollisionType::TRIANGLE_PRECISE ? "TRIANGLE_PRECISE"
                     : afterInstanceType == CollisionType::IMPROVED_AABB  ? "IMPROVED_AABB"
                     : afterInstanceType == CollisionType::AABB_ONLY      ? "AABB_ONLY"
                     : afterInstanceType == CollisionType::OCTREE_ONLY    ? "OCTREE_ONLY"
                                                                          : "OTHER",
                     modelName.c_str());

            TraceLog(LOG_INFO,
                     "Built OCTREE collision for instance at (%.2f, %.2f, %.2f)",
                     position.x, position.y, position.z);
        }
        else
        {
            // Reached limit, use AABB collision instead
            TraceLog(LOG_WARNING,
                     "Reached limit of %d precise collisions for model '%s', using AABB for "
                     "remaining instances",
                     MAX_PRECISE_COLLISIONS_PER_MODEL, modelName.c_str());

            // Use AABB collision (fallback to else branch logic)
            instanceCollision = *cachedCollision;
            Vector3 cachedCenter = cachedCollision->GetCenter();
            Vector3 cachedSize = cachedCollision->GetSize();

            // Apply transformation to center and scale
            Vector3 transformedCenter = Vector3Add(Vector3Scale(cachedCenter, scale), position);
            Vector3 scaledSize = Vector3Scale(cachedSize, scale);

            instanceCollision.Update(transformedCenter, scaledSize);

            TraceLog(LOG_INFO, "Used AABB collision fallback for instance at (%.2f, %.2f, %.2f)",
                     position.x, position.y, position.z);
        }
    }
    else
    {
        // For AABB collision, apply both scale and position transform
        instanceCollision = *cachedCollision;
        Vector3 cachedCenter = cachedCollision->GetCenter();
        Vector3 cachedSize = cachedCollision->GetSize();

        // Apply transformation to center and scale
        Vector3 transformedCenter = Vector3Add(Vector3Scale(cachedCenter, scale), position);
        Vector3 scaledSize = Vector3Scale(cachedSize, scale);

        instanceCollision.Update(transformedCenter, scaledSize);

        TraceLog(LOG_INFO, "Transformed AABB collision for instance at (%.2f, %.2f, %.2f)",
                 position.x, position.y, position.z);
    }

    // Add the instance collision to collision manager
    m_collisionManager.AddCollider(instanceCollision);

    TraceLog(LOG_INFO, "Successfully created instance collision for '%s' at (%.2f, %.2f, %.2f)",
             modelName.c_str(), position.x, position.y, position.z);
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
            // // Check for stuck marker first
            // if (responseLength > 500.0f) {
            //     TraceLog(LOG_ERROR, "🚨 STUCK MARKER DETECTED in Engine (%.2f) - player extracting", responseLength);
            //     // Let the player handle extraction - don't apply the response
            //     return;
            // }
            
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
    // Reset player if they fall significantly below the world floor
    // Give some buffer to prevent immediate respawn due to collision issues
    const float RESPAWN_THRESHOLD = PhysicsComponent::WORLD_FLOOR_Y - 5.0f;
    
    if (m_player.GetPlayerPosition().y < RESPAWN_THRESHOLD)
    {
        TraceLog(LOG_WARNING, "Player fell below world bounds (%.1f), respawning at (%.1f, %.1f, %.1f)...",
                 RESPAWN_THRESHOLD, 
                 Player::DEFAULT_SPAWN_POSITION.x, 
                 Player::DEFAULT_SPAWN_POSITION.y, 
                 Player::DEFAULT_SPAWN_POSITION.z);
        m_player.SetPlayerPosition(Player::DEFAULT_SPAWN_POSITION);
        
        // Reset physics state to prevent immediate falling
        auto& physics = const_cast<PhysicsComponent&>(m_player.GetPhysics());
        physics.SetGroundLevel(true);
        physics.SetVelocity({0, 0, 0});
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
