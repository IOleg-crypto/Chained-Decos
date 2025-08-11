// Created by I#Oleg
//
#include <Engine/Engine.h>
#include <Menu/Menu.h>
#include <raylib.h>
// Include ImGui with adapter
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
}

Engine::~Engine()
{
    rlImGuiShutdown(); // cleans up ImGui
    CloseWindow();
}

void Engine::Init()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_screenX, m_screenY, m_windowName.c_str());
    SetTargetFPS(144);
    rlImGuiSetup(true); // init ImGui
    InitImGuiFont();
    m_models.LoadModelsFromJson(PROJECT_ROOT_DIR "/src/models.json");
    InitCollisions();
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
    if (IsKeyPressed(KEY_ESCAPE))
    {
        ToggleMenu();
    }

    HandleKeyboardShortcuts();

    if (const ImGuiIO &io = ImGui::GetIO(); !io.WantCaptureMouse)
    {
        m_player.Update();
    }

    m_player.ApplyGravityForPlayer(m_collisionManager);
    m_player.UpdatePlayerBox();

    Vector3 response = {};
    bool isColliding = m_collisionManager.CheckCollision(m_player.GetCollision(), response);

    if (isColliding)
    {
        m_player.Move(response);
    }
    // If player falls below ground level, reset position
    if (m_player.GetPlayerPosition().y < 0.f)
    {
        m_player.SetPlayerPosition({0, 10, 0});
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
    DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){800.0f, 800.0f}, LIGHTGRAY);
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

void Engine::InitCollisions()
{
    Model &arenaModel = m_models.GetModelByName("arc");
    Collision m_arenaCollision;
    m_arenaCollision.GetMeshModelCollision(&arenaModel);
    m_collisionManager.AddCollider(m_arenaCollision);

    // Vector3 groundPos = {0, -0.5f, 0};
    // Vector3 groundSize = {800.f, 2.0f, 800.f};
    // Collision groundCollider(groundPos, groundSize);
    // m_collisionManager.AddCollider(groundCollider);
}

void Engine::InitInput()
{
    Camera &camera = m_player.GetCameraController()->GetCamera();
    int &cameraMode = m_player.GetCameraController()->GetCameraMode();

    m_manager.RegisterAction(KEY_FOUR, [this]() { ToggleFullscreen(); });
    m_manager.RegisterAction(KEY_ONE,
                             [this, &camera, &cameraMode]()
                             {
                                 cameraMode = CAMERA_FREE;
                                 camera.up = {0, 1, 0};
                             });
    m_manager.RegisterAction(KEY_TWO, [this]() { m_showDebug = !m_showDebug; });

    m_manager.RegisterAction(KEY_C, [this]() { m_showCollisionDebug = !m_showCollisionDebug; });

    m_manager.RegisterAction(KEY_ONE,
                             [this]
                             {
                                 m_showMenu = true;
                                 EnableCursor();
                             });
}

void Engine::HandleKeyboardShortcuts() const { m_manager.ProcessInput(); }
