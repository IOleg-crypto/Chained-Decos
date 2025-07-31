// Created by I#Oleg
//
#include <Engine/Engine.h>
// Include ImGui with adapter
#include <imgui.h>
#include <rlImGui.h>
#include <rcamera.h> // optional, consider removing if using precompiled raylib


//Just used for keyboard binding , lol
#include <glfw3.h>

Engine::Engine(const int screenX, const int screenY)
    : m_screenX(screenX), m_screenY(screenY), m_WindowName("Chained Decos"), m_shouldExit(false) {
    if (m_screenX < 0 || m_screenY < 0) {
        TraceLog(LOG_WARNING, "[Screen] Invalid screen size: %d x %d. Setting default size 800x600.", m_screenX,
                 m_screenY);
        m_screenX = 800;
        m_screenY = 600;
    }
}

Engine::~Engine() {
    rlImGuiShutdown();        // cleans up ImGui
    CloseWindow();
}

void Engine::Init() const {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_screenX, m_screenY, m_WindowName.c_str());
    HideCursor();
    SetTargetFPS(60);
    rlImGuiSetup(true); // init ImGui
}

void Engine::Run() {
    m_models.LoadModelsFromJson(std::string(GetWorkingDirectory()) + "/src/models.json");

    while (!WindowShouldClose()) {
        Update();
        Render();
        if (m_shouldExit) {
            break;
        };
    }
}

void Engine::Update() {
    KeyboardShortcut();
    if (const ImGuiIO& io = ImGui::GetIO(); !io.WantCaptureMouse) {
        m_player.Update();
    }
}

void Engine::Render() {
    BeginDrawing();
    ClearBackground(BLUE);

    if (showMenu) {
        m_menu.Update();
        m_menu.Render();
    } else {
        BeginMode3D(m_player.getCameraController()->getCamera());
        DrawScene3D();
        EndMode3D();

    }

    if (m_showDebug) {
        TraceLog(LOG_DEBUG, "Create ImGui Window for DEBUG");
        DrawDebugInfo(m_player.getCameraController()->getCamera(), m_player.getCameraController()->GetCameraMode());
    }

    EndDrawing();

    // handle menu actions after frame
    switch (m_menu.GetAction()) {
        case MenuAction::StartGame:
            showMenu = false;
            InitInput();
            m_menu.ResetAction();
            break;
        case MenuAction::OpenOptions:
            m_menu.ResetAction();
            break;
        case MenuAction::ExitGame:
            m_shouldExit = true;
            break;
        default: break;
    }
}

void Engine::DrawScene3D() {
    DrawGrid(50, 5.0f);
    DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 500.0f, 500.0f }, LIGHTGRAY); // Draw ground
    // Draw player cube
    if (m_player.getCameraController()->GetCameraMode() == CAMERA_THIRD_PERSON)
    {
        DrawCube(m_player.getCameraController()->getCamera().target, 0.5f, 0.5f, 0.5f, PURPLE);
        DrawCubeWires(m_player.getCameraController()->getCamera().target, 0.5f, 0.5f, 0.5f, DARKPURPLE);
    }
    m_models.DrawAllModels();
}

void Engine::DrawDebugInfo(const Camera &camera , const int &cameraMode) {
    rlImGuiBegin();
    ImGui::SetNextWindowSize(ImVec2(384, 256), ImGuiCond_Always);
    if (ImGui::Begin("Debug info") , nullptr , ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground) {
        ImGui::Text("Camera status:");
        ImGui::Text("- Mode: %s", (cameraMode == CAMERA_FREE) ? "FREE" :
                                   (cameraMode == CAMERA_FIRST_PERSON) ? "FIRST_PERSON" :
                                   (cameraMode == CAMERA_THIRD_PERSON) ? "THIRD_PERSON" :
                                   (cameraMode == CAMERA_ORBITAL) ? "ORBITAL" : "CUSTOM");
        ImGui::Text("- Projection: %s", (camera.projection == CAMERA_PERSPECTIVE) ? "PERSPECTIVE" :
                                         (camera.projection == CAMERA_ORTHOGRAPHIC) ? "ORTHOGRAPHIC" : "CUSTOM");
        ImGui::Text("- Position: (%.3f, %.3f, %.3f)", camera.position.x, camera.position.y, camera.position.z);
        ImGui::Text("- Target:   (%.3f, %.3f, %.3f)", camera.target.x, camera.target.y, camera.target.z);
        ImGui::Text("- Up:       (%.3f, %.3f, %.3f)", camera.up.x, camera.up.y, camera.up.z);
        ImGui::Text("FPS: %d", GetFPS());
        ImGui::Text("Player speed : %f" , m_player.GetSpeed());
    }
    ImGui::End();
    rlImGuiEnd();
}

void Engine::InitInput() {
    Camera &camera = m_player.getCameraController()->getCamera();
    int &cameraMode = m_player.getCameraController()->GetCameraMode();

    manager.RegisterAction(KEY_F5, [this]() {
        ToggleFullscreen();
    });

    manager.RegisterAction(KEY_ONE, [this, &camera, &cameraMode]() {
        cameraMode = CAMERA_FREE;
        camera.up = {0, 1, 0};
    });

    manager.RegisterAction(KEY_TWO, [this, &camera, &cameraMode]() {
        cameraMode = CAMERA_FIRST_PERSON;
        camera.up = {0, 1, 0};
    });

    manager.RegisterAction(KEY_THREE, [this, &camera, &cameraMode]() {
        cameraMode = CAMERA_THIRD_PERSON;
        camera.up = {0, 1, 0};
    });

    manager.RegisterAction(KEY_FOUR, [this, &camera, &cameraMode]() {
        cameraMode = CAMERA_ORBITAL;
        camera.up = {0, 1, 0};
    });

    manager.RegisterAction(KEY_P, [this, &camera, &cameraMode]() {
        if (camera.projection == CAMERA_PERSPECTIVE) {
            cameraMode = CAMERA_THIRD_PERSON;
            camera.position = {0, 2, -100};
            camera.target = {0, 2, 0};
            camera.up = {0, 1, 0};
            camera.projection = CAMERA_ORTHOGRAPHIC;
            camera.fovy = 20;
            CameraYaw(&camera, -135 * DEG2RAD, true);
            CameraPitch(&camera, -45 * DEG2RAD, true, true, false);
        } else if (camera.projection == CAMERA_ORTHOGRAPHIC) {
            cameraMode = CAMERA_THIRD_PERSON;
            camera.position = {0, 2, 10};
            camera.target = {0, 2, 0};
            camera.up = {0, 1, 0};
            camera.projection = CAMERA_PERSPECTIVE;
            camera.fovy = 60;
        }
    });

    manager.RegisterAction(KEY_FIVE, [this]() {
        m_showDebug = !m_showDebug;
    });

    manager.RegisterAction(GLFW_KEY_1, [this] {
        showMenu = true;
    });
}


void Engine::KeyboardShortcut() const {
    manager.ProcessInput();
}
