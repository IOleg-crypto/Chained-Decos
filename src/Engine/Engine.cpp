// Created by I#Oleg
//
#include "Engine.h"
#include "Menu/Menu.h"
#include "raylib.h"
// Include ImGui with adapter
#include <Collision/CollisionSystem.h>
#include <imgui.h>
#include <rcamera.h>
#include <rlImGui.h>

Engine::Engine(const int screenX, const int screenY)
    : m_screenX(screenX), m_screenY(screenY), m_windowName("Chained Decos")
{
    if (m_screenX < 0 || m_screenY < 0)
    {
        TraceLog(LOG_WARNING,
                 "[Screen] Invalid screen size: %d x %d. Setting default size 800x600.", m_screenX,
                 m_screenY);
        m_screenX = 800;
        m_screenY = 600;
    }
}

Engine::~Engine()
{
    rlImGuiShutdown(); // cleans up ImGui
    CloseWindow();
}

void Engine::Init() const
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_screenX, m_screenY, m_windowName.c_str());
    SetTargetFPS(144);
    rlImGuiSetup(true); // init ImGui
    InitImGuiFont();
}

void Engine::Run()
{
    m_models.LoadModelsFromJson(PROJECT_ROOT_DIR "/src/models.json");

    while (!WindowShouldClose())
    {
        Update();
        Render();
        if (m_shouldExit)
        {
            break;
        };
    }
}

void Engine::Update()
{
    HandleKeyboardShortcuts();
    if (const ImGuiIO &io = ImGui::GetIO(); !io.WantCaptureMouse)
    {
        m_player.Update();
        UpdateCollisions();
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
        if (m_showCollisionDebug)
        {
            DrawCollisionDebug();
        }
        EndMode3D();
    }

    if (m_showDebug)
    {
        TraceLog(LOG_DEBUG, "Create ImGui Window for DEBUG");
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
    Model &model = m_models.GetModelByName("player");

    // Get player position from camera but place model on ground (y = 0)
    Vector3 playerPosition = m_player.GetCameraController()->GetCamera().position;
    Vector3 modelPosition = {playerPosition.x, 0.0f, playerPosition.z}; // Place on ground

    if (m_player.GetCameraController()->GetCameraMode() == CAMERA_THIRD_PERSON)
    {
        model.transform = m_player.GetPlayerRotation();
        DrawModel(model, m_player.GetCameraController()->GetCamera().target, 1, GRAY);
    }
    else
    {
        model.transform = m_player.GetPlayerRotation();
        DrawModel(model, modelPosition, 0.5, GRAY);
    }
}

void Engine::DrawScene3D()
{
    DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){800.0f, 800.0f}, LIGHTGRAY); // Draw ground
    LoadPlayerModel();
    // Draw additional geometric shapes for collision testing
    DrawGeometricShapes();

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
    }
    ImGui::End();
    rlImGuiEnd();
}

void Engine::InitInput()
{
    Camera &camera = m_player.GetCameraController()->GetCamera();
    int &cameraMode = m_player.GetCameraController()->GetCameraMode();

    m_manager.RegisterAction(KEY_F5, [this]() { ToggleFullscreen(); });
    m_manager.RegisterAction(KEY_ONE,
                             [this, &camera, &cameraMode]()
                             {
                                 cameraMode = CAMERA_FREE;
                                 camera.up = {0, 1, 0};
                             });

    m_manager.RegisterAction(KEY_TWO,
                             [this, &camera, &cameraMode]() { cameraMode = CAMERA_FIRST_PERSON; });

    m_manager.RegisterAction(KEY_THREE,
                             [this, &camera, &cameraMode]() { cameraMode = CAMERA_THIRD_PERSON; });

    m_manager.RegisterAction(KEY_FOUR,
                             [this, &camera, &cameraMode]()
                             {
                                 cameraMode = CAMERA_ORBITAL;
                                 camera.up = {0, 1, 0};
                             });

    m_manager.RegisterAction(KEY_P,
                             [this, &camera, &cameraMode]()
                             {
                                 if (camera.projection == CAMERA_PERSPECTIVE)
                                 {
                                     cameraMode = CAMERA_THIRD_PERSON;
                                     camera.position = {0, 2, -100};
                                     camera.target = {0, 2, 0};
                                     camera.up = {0, 1, 0};
                                     camera.projection = CAMERA_ORTHOGRAPHIC;
                                     camera.fovy = 20;
                                     CameraYaw(&camera, -135 * DEG2RAD, true);
                                     CameraPitch(&camera, -45 * DEG2RAD, true, true, false);
                                 }
                                 else if (camera.projection == CAMERA_ORTHOGRAPHIC)
                                 {
                                     cameraMode = CAMERA_THIRD_PERSON;
                                     camera.position = {0, 2, 10};
                                     camera.target = {0, 2, 0};
                                     camera.up = {0, 1, 0};
                                     camera.projection = CAMERA_PERSPECTIVE;
                                     camera.fovy = 60;
                                 }
                             });

    m_manager.RegisterAction(KEY_FIVE, [this]() { m_showDebug = !m_showDebug; });

    m_manager.RegisterAction(KEY_C, [this]() { m_showCollisionDebug = !m_showCollisionDebug; });

    m_manager.RegisterAction(KEY_ONE, [this] { m_showMenu = true; });
}

void Engine::HandleKeyboardShortcuts() const { m_manager.ProcessInput(); }

void Engine::UpdateCollisions()
{
    // Get all model colliders
    std::vector<CollisionComponent *> modelColliders = GetModelColliders();

    // Check and handle collisions between player and models
    m_player.CheckAndHandleCollisions(modelColliders);
}

void Engine::DrawCollisionDebug()
{
    // Draw player collision bounds
    CollisionComponent &playerCollision = m_player.GetCollisionComponent();
    if (playerCollision.isActive)
    {
        if (playerCollision.type == CollisionComponent::SPHERE ||
            playerCollision.type == CollisionComponent::BOTH)
        {
            CollisionSystem::DrawCollisionSphere(playerCollision.sphere, RED);
        }

        if (playerCollision.type == CollisionComponent::AABB ||
            playerCollision.type == CollisionComponent::BOTH)
        {
            CollisionSystem::DrawCollisionBox(playerCollision.box, RED);
        }
    }

    // Draw model collision bounds
    std::vector<CollisionComponent *> modelColliders = GetModelColliders();
    for (const auto *collider : modelColliders)
    {
        if (!collider || !collider->isActive)
            continue;

        Color color = collider->isStatic ? GREEN : YELLOW;

        if (collider->type == CollisionComponent::SPHERE ||
            collider->type == CollisionComponent::BOTH)
        {
            CollisionSystem::DrawCollisionSphere(collider->sphere, color);
        }

        if (collider->type == CollisionComponent::AABB ||
            collider->type == CollisionComponent::BOTH)
        {
            CollisionSystem::DrawCollisionBox(collider->box, color);
        }
    }

    // Draw collision debug info
    DrawText("Collision Debug Mode", 10, GetScreenHeight() - 100, 20, WHITE);
    DrawText("Red = Player", 10, GetScreenHeight() - 75, 16, RED);
    DrawText("Green = Static Objects", 10, GetScreenHeight() - 55, 16, GREEN);
    DrawText("Yellow = Dynamic Objects", 10, GetScreenHeight() - 35, 16, YELLOW);
    DrawText("Press 'C' to toggle", 10, GetScreenHeight() - 15, 16, GRAY);
}

std::vector<CollisionComponent *> Engine::GetModelColliders() { return m_models.GetAllColliders(); }

void Engine::DrawGeometricShapes()
{
    // Draw various geometric shapes for collision testing

    // Cubes at different positions
    DrawCube({15.0f, 2.0f, 5.0f}, 2.0f, 2.0f, 2.0f, RED);
    DrawCubeWires({15.0f, 2.0f, 5.0f}, 2.0f, 2.0f, 2.0f, MAROON);

    DrawCube({-15.0f, 1.5f, -5.0f}, 3.0f, 1.0f, 3.0f, BLUE);
    DrawCubeWires({-15.0f, 1.5f, -5.0f}, 3.0f, 1.0f, 3.0f, DARKBLUE);

    // Spheres
    DrawSphere({12.0f, 3.0f, -8.0f}, 1.5f, GREEN);
    DrawSphereWires({12.0f, 3.0f, -8.0f}, 1.5f, 8, 8, DARKGREEN);

    DrawSphere({-12.0f, 2.0f, 8.0f}, 2.0f, YELLOW);
    DrawSphereWires({-12.0f, 2.0f, 8.0f}, 2.0f, 8, 8, GOLD);

    // Cylinders
    DrawCylinder({8.0f, 0.0f, -12.0f}, 1.0f, 2.0f, 4.0f, 16, PURPLE);
    DrawCylinderWires({8.0f, 0.0f, -12.0f}, 1.0f, 2.0f, 4.0f, 16, VIOLET);

    DrawCylinder({-8.0f, 0.0f, 12.0f}, 1.5f, 1.5f, 3.0f, 16, ORANGE);
    DrawCylinderWires({-8.0f, 0.0f, 12.0f}, 1.5f, 1.5f, 3.0f, 16, BROWN);

    // More complex shapes
    // Rectangular prisms (elongated cubes)
    DrawCube({20.0f, 1.0f, 0.0f}, 1.0f, 6.0f, 1.0f, PINK);
    DrawCubeWires({20.0f, 1.0f, 0.0f}, 1.0f, 6.0f, 1.0f, MAGENTA);

    DrawCube({-20.0f, 1.0f, 0.0f}, 4.0f, 1.0f, 1.0f, LIME);
    DrawCubeWires({-20.0f, 1.0f, 0.0f}, 4.0f, 1.0f, 1.0f, GREEN);

    // Small cubes in a pattern
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            Vector3 pos = {-25.0f + i * 2.0f, 0.5f + j * 1.0f, 15.0f};
            Color color =
                (Color){(unsigned char)(50 + i * 40), (unsigned char)(100 + j * 50), 150, 255};
            DrawCube(pos, 0.8f, 0.8f, 0.8f, color);
            DrawCubeWires(pos, 0.8f, 0.8f, 0.8f, BLACK);
        }
    }

    // Spheres in a line
    for (int i = 0; i < 6; i++)
    {
        Vector3 pos = {25.0f, 1.0f + i * 0.5f, -15.0f + i * 2.0f};
        Color color =
            (Color){255, (unsigned char)(50 + i * 30), (unsigned char)(100 + i * 25), 255};
        DrawSphere(pos, 0.6f + i * 0.1f, color);
        DrawSphereWires(pos, 0.6f + i * 0.1f, 8, 8, BLACK);
    }

    // Tall cylinders as pillars
    DrawCylinder({0.0f, 0.0f, 25.0f}, 1.0f, 1.0f, 8.0f, 12, BEIGE);
    DrawCylinderWires({0.0f, 0.0f, 25.0f}, 1.0f, 1.0f, 8.0f, 12, BROWN);

    DrawCylinder({5.0f, 0.0f, 25.0f}, 0.8f, 0.8f, 6.0f, 12, LIGHTGRAY);
    DrawCylinderWires({5.0f, 0.0f, 25.0f}, 0.8f, 0.8f, 6.0f, 12, GRAY);

    DrawCylinder({-5.0f, 0.0f, 25.0f}, 1.2f, 1.2f, 10.0f, 12, DARKGRAY);
    DrawCylinderWires({-5.0f, 0.0f, 25.0f}, 1.2f, 1.2f, 10.0f, 12, BLACK);
}
