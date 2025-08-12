// Created by I#Oleg
//
#include <Engine/Engine.h>
#include <Menu/Menu.h>
#include <raylib.h>
// Include ImGui with adapter
#include <Collision/CollisionDebugRenderer.h>
#include <Collision/CollisionSystem.h>
#include <imgui.h>
#include <rcamera.h>
#include <rlImGui.h>

Engine::Engine(const int screenX, const int screenY)
    : m_screenX(screenX), m_screenY(screenY), m_windowName("Chained Decos"), m_usePlayerModel(true)
{
    if (m_screenX < 0 || m_screenY < 0)
    {
        TraceLog(LOG_WARNING,
                 "[Screen] Invalid screen size: %d x %d. Setting default size 800x600.", m_screenX,
                 m_screenY);
        m_screenX = 800;
        m_screenY = 600;
    }

    m_collisionDebugRenderer = new CollisionDebugRenderer();
    TraceLog(LOG_INFO, "CollisionDebugRenderer initialized");
}

Engine::~Engine()
{
    delete m_collisionDebugRenderer;
    m_collisionDebugRenderer = nullptr;

    rlImGuiShutdown(); // cleans up ImGui
    CloseWindow();

    TraceLog(LOG_INFO, "Engine destructor completed");
}

void Engine::Init()
{
    // Configure window flags
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);

    // Initialize window
    InitWindow(m_screenX, m_screenY, m_windowName.c_str());
    SetTargetFPS(144);

    // Initialize ImGui
    rlImGuiSetup(true);
    InitImGuiFont();

    TraceLog(LOG_INFO, "Loading models from: %s", PROJECT_ROOT_DIR "/src/models.json");
    m_models.SetCacheEnabled(true);
    m_models.SetMaxCacheSize(20);
    m_models.EnableLOD(false);
    // Load models from JSON file (if model not exist - it crashing)
    m_models.LoadModelsFromJson(PROJECT_ROOT_DIR "/src/models.json");
    m_models.PrintStatistics();

    // Initialize game systems
    InitCollisions();

    TraceLog(LOG_INFO, "Engine initialization complete!");
}

void Engine::Run()
{
    while (!WindowShouldClose())
    {
        Update();
        Render();
        if (m_shouldExit)
        {
            break;
        }
    }
}

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
    // Reset player if they fall below the world
    if (m_player.GetPlayerPosition().y < -1.0f) // Changed threshold from 0 to -50
    {
        TraceLog(LOG_WARNING, "Player fell below world bounds, respawning...");
        m_player.SetPlayerPosition({2, 50, 150});
    }
}

void Engine::Render()
{
    BeginDrawing();
    ClearBackground(BLUE);

    if (m_showMenu)
    {
        m_menu.Update();
        m_menu.Render();
    }
    else
    {
        BeginMode3D(m_player.GetCameraController()->GetCamera());
        DrawScene3D();
        LoadPlayerModel();
        m_player.UpdatePlayerCollision();

        if (m_showCollisionDebug)
        {
            RenderCollisionDebug();
        }

        EndMode3D();
    }

    if (m_showDebug)
    {
        DrawDebugInfo(m_player.GetCameraController()->GetCamera(),
                      m_player.GetCameraController()->GetCameraMode());
    }
    EndDrawing();

    // handle menu actions after frame
    switch (m_menu.GetAction())
    {
    case MenuAction::StartGame:
        m_showMenu = false;
        InitInput();
        HideCursor(); // Hide mouse cursor when game starts
        m_menu.ResetAction();
        break;
    case MenuAction::OpenOptions:
        m_menu.ResetAction();
        break;
    case MenuAction::ExitGame:
        m_menu.ResetAction();
        m_shouldExit = true;
        break;
    default:
        break;
    }
}

void Engine::LoadPlayerModel()
{
    Model &m_playerModel = m_models.GetModelByName("player");
    m_playerModel.transform = MatrixRotateY(DEG2RAD * m_player.GetRotationY());

    Vector3 adjustedPos = m_player.GetPlayerPosition();
    adjustedPos.y -= 1.2f;

    DrawModel(m_playerModel, adjustedPos, 1.0f, WHITE);
    DrawBoundingBox(m_player.GetPlayerBoundingBox(), GREEN);
}

void Engine::DrawScene3D()
{
    DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){800.0f, 800.0f}, LIGHTGRAY); // Draw ground
    m_models.DrawAllModels();
}

void Engine::InitImGuiFont() const
{
    const ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(PROJECT_ROOT_DIR "/resources/font/Lato/Lato-Black.ttf", 16.0f);
    io.Fonts->Build();
}

void Engine::DrawDebugInfo(const Camera &camera, const int &cameraMode)
{
    rlImGuiBegin();
    ImGui::SetNextWindowSize(ImVec2(384, 256), ImGuiCond_Always);
    if (ImGui::Begin("Debug info", nullptr, ImGuiWindowFlags_NoResize))
    {
        ImGui::Text("Camera status:");
        ImGui::Text("- Mode: %s", (cameraMode == CAMERA_FREE)           ? "FREE"
                                  : (cameraMode == CAMERA_FIRST_PERSON) ? "FIRST_PERSON"
                                  : (cameraMode == CAMERA_THIRD_PERSON) ? "THIRD_PERSON"
                                  : (cameraMode == CAMERA_ORBITAL)      ? "ORBITAL"
                                                                        : "CUSTOM");
        ImGui::Text("- Projection: %s", (camera.projection == CAMERA_PERSPECTIVE) ? "PERSPECTIVE"
                                        : (camera.projection == CAMERA_ORTHOGRAPHIC)
                                            ? "ORTHOGRAPHIC"
                                            : "CUSTOM");
        ImGui::Text("- Position: (%.3f, %.3f, %.3f)", camera.position.x, camera.position.y,
                    camera.position.z);
        ImGui::Text("- Target:   (%.3f, %.3f, %.3f)", camera.target.x, camera.target.y,
                    camera.target.z);
        ImGui::Text("- Up:       (%.3f, %.3f, %.3f)", camera.up.x, camera.up.y, camera.up.z);
        ImGui::Text("FPS: %d", GetFPS());
        ImGui::Text("Player speed : %f", m_player.GetSpeed());

        ImGui::Separator();
        ImGui::Text("Model Manager:");

        const auto &stats = m_models.GetLoadingStats();
        ImGui::Text("- Models loaded: %d/%d (%.1f%%)", stats.loadedModels, stats.totalModels,
                    stats.GetSuccessRate() * 100);
        ImGui::Text("- Total instances: %d", stats.totalInstances);
        ImGui::Text("- Loading time: %.2fs", stats.loadingTime);

        if (stats.failedModels > 0)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "- Failed: %d models",
                               stats.failedModels);
        }

        if (ImGui::Button("Print Full Stats"))
        {
            m_models.PrintStatistics();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cache Info"))
        {
            m_models.PrintCacheInfo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cleanup Cache"))
        {
            m_models.CleanupUnusedModels();
        }
    }
    ImGui::End();
    rlImGuiEnd();
}

void Engine::InitCollisions()
{
    // Clear existing collisions
    m_collisionManager.ClearColliders();

    TraceLog(LOG_INFO, "Initializing collision system...");

    try
    {
        // 1. Ground collision (large plane)
        Collision groundCollision{{0, 0, 0}, {1000, 1, 1000}};
        m_collisionManager.AddCollider(groundCollision);
        TraceLog(LOG_INFO, "Added ground collision");

        // 2. Arena model collision
        Model &arenaModel = m_models.GetModelByName("arc");

        Collision arenaCollision;
        arenaCollision.CalculateFromModel(&arenaModel, arenaModel.transform);
        m_collisionManager.AddCollider(arenaCollision);
        TraceLog(LOG_INFO, "Added arena collision");

        TraceLog(LOG_INFO, "Collision system initialized with %zu colliders",
                 m_collisionManager.GetColliders().size());
    }
    catch (const std::exception &e)
    {
        TraceLog(LOG_ERROR, "Failed to initialize collisions: %s", e.what());
    }
}

void Engine::InitInput()
{
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
                                 m_showDebug = !m_showDebug;
                                 TraceLog(LOG_INFO, "Debug info: %s", m_showDebug ? "ON" : "OFF");
                             });

    m_manager.RegisterAction(KEY_F3,
                             [this]()
                             {
                                 m_showCollisionDebug = !m_showCollisionDebug;
                                 TraceLog(LOG_INFO, "F3 pressed! Collision debug: %s",
                                          m_showCollisionDebug ? "ON" : "OFF");
                             });
    m_manager.RegisterAction(KEY_F4, [this]() { m_shouldExit = true; });
    m_manager.RegisterAction(
        KEY_F10,
        [this]()
        {
            // Force show collision debug for 1 frame
            TraceLog(LOG_INFO, "F10 pressed - forcing collision debug ON for next frame");
            m_showCollisionDebug = true;
        });

    // m_manager.RegisterAction(KEY_F5, [this]() { LoadAdditionalModels(); });

    // m_manager.RegisterAction(KEY_F6, [this]() { SpawnEnemies(); });

    // m_manager.RegisterAction(KEY_F7, [this]() { SpawnPickups(); });

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

                                 //  auto enemies = m_models.GetInstancesByTag("enemy");
                                 //  auto pickups = m_models.GetInstancesByTag("pickup");
                                 auto decorations = m_models.GetInstancesByTag("decoration");

                                 //  TraceLog(LOG_INFO, "=== Instances by Tags ===");
                                 //  TraceLog(LOG_INFO, "  - Enemies: %zu", enemies.size());
                                 //  TraceLog(LOG_INFO, "  - Pickups: %zu", pickups.size());
                                 TraceLog(LOG_INFO, "  - Decorations: %zu", decorations.size());
                             });
}

void Engine::HandleKeyboardShortcuts() const { m_manager.ProcessInput(); }

void Engine::RenderCollisionDebug()
{
    DrawCubeWires({0, 5, 0}, 2, 2, 2, YELLOW);

    const auto &colliders = m_collisionManager.GetColliders();

    m_collisionDebugRenderer->RenderAllCollisions(colliders);
    m_collisionDebugRenderer->RenderPlayerCollision(m_player.GetCollision());

    TraceLog(LOG_DEBUG, "Collision debug rendered via CollisionDebugRenderer");
}

// UNUSED - COULD BE USED IN FUTURE
void Engine::LoadAdditionalModels()
{
    TraceLog(LOG_INFO, "Loading additional models dynamically...");
    if (m_models.LoadSingleModel("enemy", "/resources/enemy.glb", true))
    {
        TraceLog(LOG_INFO, "Enemy model loaded successfully");
        SpawnEnemies();
    }

    if (m_models.LoadSingleModel("pickup", "/resources/pickup.glb", true))
    {
        TraceLog(LOG_INFO, "Pickup model loaded successfully");
        SpawnPickups();
    }
}

void Engine::SpawnEnemies()
{
    TraceLog(LOG_INFO, "Spawning enemies around the arena...");

    for (int i = 0; i < 5; i++)
    {
        float angle = (2 * PI / 5) * i;
        float radius = 15.0f;

        ModelInstanceConfig enemyConfig;
        enemyConfig.position = {cos(angle) * radius, 1.0f, sin(angle) * radius};
        enemyConfig.rotation = {0, angle * RAD2DEG, 0};
        enemyConfig.scale = 1.2f;
        enemyConfig.color = RED;
        enemyConfig.tag = "enemy";
        enemyConfig.spawn = true;

        if (m_models.AddInstanceEx("enemy", enemyConfig))
        {
            TraceLog(LOG_INFO, "Spawned enemy %d at angle %.1f degrees", i + 1, angle * RAD2DEG);
        }
    }
}

void Engine::SpawnPickups()
{
    TraceLog(LOG_INFO, "Spawning pickups...");

    for (int i = 0; i < 8; i++)
    {
        ModelInstanceConfig pickupConfig;
        pickupConfig.position =
            Vector3{(float)(GetRandomValue(-10, 10)), 0.5f, (float)(GetRandomValue(-10, 10))};
        pickupConfig.scale = 0.5f;
        pickupConfig.color = YELLOW;
        pickupConfig.tag = "pickup";
        pickupConfig.spawn = true;

        if (m_models.AddInstanceEx("pickup", pickupConfig))
        {
            TraceLog(LOG_INFO, "Spawned pickup %d", i + 1);
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
}

void Engine::ToggleMenu() { m_showMenu = !m_showMenu; }
