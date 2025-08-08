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
    : m_screenX(screenX), m_screenY(screenY), m_windowName("Chained Decos"), m_playerModelMesh(),
      m_usePlayerModel(true)
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
    InitCollisions();
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
        m_player.UpdateCollision();
        EndMode3D();
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
    m_playerModelMesh = m_models.GetModelByName("player");
    m_playerModelMesh.transform = m_player.GetPlayerRotation();

    Vector3 adjustedPos = m_player.GetPlayerPosition();
    adjustedPos.y -= 0.9f;

    DrawModel(m_playerModelMesh, adjustedPos, 0.5f, WHITE);
}

void Engine::DrawScene3D()
{
    DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){800.0f, 800.0f}, LIGHTGRAY);

    DrawCube((Vector3){0, 0, 60}, 40, 10, 40, RED);
    DrawCube((Vector3){0, 0, 100}, 10, 2, 10, GREEN);
    DrawCube((Vector3){30, 0, 30}, 10, 5, 10, DARKGRAY);
    DrawCube((Vector3){-50, 0, 20}, 20, 4, 20, BROWN);
    DrawCube((Vector3){20, 5, 60}, 10, 2, 10, SKYBLUE);
    DrawCube((Vector3){-30, 8, 70}, 6, 2, 6, PURPLE);

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
    m_collisionManager.ClearColliders();

    Vector3 pos1 = {0, 0, 60.f};
    Vector3 size1 = {40.f, 10.f, 40.f};
    Collision collider1(pos1, size1);
    m_collisionManager.AddCollider(collider1);

    Vector3 pos2 = {0, 0, 100.f};
    Vector3 size2 = {10.f, 2.f, 10.f};
    Collision collider2(pos2, size2);
    m_collisionManager.AddCollider(collider2);

    Vector3 pos3 = {30, 0, 30};
    Vector3 size3 = {10.f, 5.f, 10.f};
    Collision collider3(pos3, size3);
    m_collisionManager.AddCollider(collider3);

    Vector3 pos4 = {-50, 0, 20};
    Vector3 size4 = {20.f, 4.f, 20.f};
    Collision collider4(pos4, size4);
    m_collisionManager.AddCollider(collider4);

    Vector3 pos5 = {20, 5, 60};
    Vector3 size5 = {10.f, 2.f, 10.f};
    Collision collider5(pos5, size5);
    m_collisionManager.AddCollider(collider5);

    Vector3 pos6 = {-30, 8, 70};
    Vector3 size6 = {6.f, 2.f, 6.f};
    Collision collider6(pos6, size6);
    m_collisionManager.AddCollider(collider6);

    Vector3 groundPos = {0, -0.5f, 0};
    Vector3 groundSize = {800.f, 1.0f, 800.f};
    Collision groundCollider(groundPos, groundSize);
    m_collisionManager.AddCollider(groundCollider);
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
    // UNUSED

    // m_manager.RegisterAction(KEY_TWO,
    //                          [this, &camera, &cameraMode]() { cameraMode = CAMERA_FIRST_PERSON;
    //                          });

    // m_manager.RegisterAction(KEY_THREE,
    //                          [this, &camera, &cameraMode]() { cameraMode = CAMERA_THIRD_PERSON;
    //                          });

    // m_manager.RegisterAction(KEY_FOUR,
    //                          [this, &camera, &cameraMode]()
    //                          {
    //                              cameraMode = CAMERA_ORBITAL;
    //                              camera.up = {0, 1, 0};
    //                          });

    // m_manager.RegisterAction(KEY_P,
    //                          [this, &camera, &cameraMode]()
    //                          {
    //                              if (camera.projection == CAMERA_PERSPECTIVE)
    //                              {
    //                                  cameraMode = CAMERA_THIRD_PERSON;
    //                                  camera.position = {0, 2, -100};
    //                                  camera.target = {0, 2, 0};
    //                                  camera.up = {0, 1, 0};
    //                                  camera.projection = CAMERA_ORTHOGRAPHIC;
    //                                  camera.fovy = 20;
    //                                  CameraYaw(&camera, -135 * DEG2RAD, true);
    //                                  CameraPitch(&camera, -45 * DEG2RAD, true, true, false);
    //                              }
    //                              else if (camera.projection == CAMERA_ORTHOGRAPHIC)
    //                              {
    //                                  cameraMode = CAMERA_THIRD_PERSON;
    //                                  camera.position = {0, 2, 10};
    //                                  camera.target = {0, 2, 0};
    //                                  camera.up = {0, 1, 0};
    //                                  camera.projection = CAMERA_PERSPECTIVE;
    //                                  camera.fovy = 60;
    //                              }
    //                          });
#ifndef DEBUG
    m_manager.RegisterAction(KEY_F5, [this]() { m_showDebug = !m_showDebug; });
#endif // DEBUG

    m_manager.RegisterAction(KEY_C, [this]() { m_showCollisionDebug = !m_showCollisionDebug; });

    m_manager.RegisterAction(KEY_ONE, [this] { m_showMenu = true; });
}

void Engine::HandleKeyboardShortcuts() const { m_manager.ProcessInput(); }
