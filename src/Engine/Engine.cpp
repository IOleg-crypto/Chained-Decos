//
// Created by I#Oleg
//
#include <rcamera.h>
#include <Engine/Engine.h>
// Include ImGui with adapter
#include <imgui.h>
#include <rlImGui.h>

Engine::Engine(const int screenX, const int screenY)
    : m_screenX(screenX), m_screenY(screenY), m_WindowName("Chained Decos") {

    if (m_screenX < 0 || m_screenY < 0) {
        TraceLog(LOG_WARNING, "[Screen] Invalid screen size: %d x %d. Setting default size 800x600.", m_screenX, m_screenY);
        m_screenX = 800;
        m_screenY = 600;
    }
}

Engine::~Engine() {
    rlImGuiShutdown();		// cleans up ImGui
    CloseWindow();
}
void Engine::Init() const {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_screenX, m_screenY, m_WindowName.c_str());
    HideCursor();
    SetTargetFPS(60);
    // Using adapter to init ImGui
    rlImGuiSetup(true);
}
void Engine::Run() {
    // Loading models
    m_models.LoadModelsFromJson(std::string(GetWorkingDirectory()) + "/src/models.json");
    // Main game loop
    while (!WindowShouldClose()) {
        Update();
        Render();
    }
}

void Engine::Update() {
    KeyboardShortcut();
}

void Engine::Render() {
    BeginDrawing();
    ClearBackground(BLUE);

    BeginMode3D(m_player.getCamera());
    DrawScene3D();
    EndMode3D();

    if (m_showDebug) {
        TraceLog(LOG_DEBUG, "Create ImGui Window for DEBUG");
        DrawDebugInfo(m_player.getCamera(), m_player.GetCameraMode() , m_showDebug);
    }

    EndDrawing();
}
void Engine::DrawScene3D() const {
    DrawGrid(50, 5.0f);
    DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 500.0f, 500.0f }, LIGHTGRAY); // Draw ground
    m_models.DrawAllModels();
}

void Engine::DrawDebugInfo(const Camera &camera , const int &cameraMode , bool &showDebugMenu) {
    rlImGuiBegin();			// starts the ImGui content mode. Make all ImGui calls after this
    ImGui::SetNextWindowSize(ImVec2(384, 256), ImGuiCond_Always);
    if (ImGui::Begin("Debug info") , showDebugMenu) {
        //DEBUG information
        ImGui::Text("Camera status:", 610, 15, 10, BLACK);
        ImGui::Text(TextFormat("- Mode: %s", (cameraMode == CAMERA_FREE) ? "FREE" :
                                          (cameraMode == CAMERA_FIRST_PERSON) ? "FIRST_PERSON" :
                                          (cameraMode == CAMERA_THIRD_PERSON) ? "THIRD_PERSON" :
                                          (cameraMode == CAMERA_ORBITAL) ? "ORBITAL" : "CUSTOM"), 610, 30, 10, BLACK);
        ImGui::Text(TextFormat("- Projection: %s", (camera.projection == CAMERA_PERSPECTIVE) ? "PERSPECTIVE" :
                                                (camera.projection == CAMERA_ORTHOGRAPHIC) ? "ORTHOGRAPHIC" : "CUSTOM"), 610, 45, 10, BLACK);
        ImGui::Text(TextFormat("- Position: (%06.3f, %06.3f, %06.3f)", camera.position.x, camera.position.y, camera.position.z), 610, 60, 10, BLACK);
        ImGui::Text(TextFormat("- Target: (%06.3f, %06.3f, %06.3f)", camera.target.x, camera.target.y, camera.target.z), 610, 75, 10, BLACK);
        ImGui::Text(TextFormat("- Up: (%06.3f, %06.3f, %06.3f)", camera.up.x, camera.up.y, camera.up.z), 610, 90, 10, BLACK);
        ImGui::Text("FPS : %d" , GetFPS());
    }
    ImGui::End();
    rlImGuiEnd();
}

void Engine::KeyboardShortcut() {
    if (IsKeyPressed(KEY_F5)) {
        ToggleFullscreen();
    }
    // Update only camera rotation (not movement) using raylib
    if (const ImGuiIO& io = ImGui::GetIO(); !io.WantCaptureMouse) {
        m_player.Update();
    }

    Camera &camera = m_player.getCamera();
    int &cameraMode = m_player.GetCameraMode();

    if (IsKeyPressed(KEY_ONE))
    {
        cameraMode = CAMERA_FREE;
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // Reset roll
    }

    if (IsKeyPressed(KEY_TWO))
    {
        cameraMode = CAMERA_FIRST_PERSON;
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // Reset roll
    }

    if (IsKeyPressed(KEY_THREE))
    {
        cameraMode = CAMERA_THIRD_PERSON;
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // Reset roll
    }

    if (IsKeyPressed(KEY_FOUR))
    {
        cameraMode = CAMERA_ORBITAL;
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f }; // Reset roll
    }

    // Switch camera projection
    if (IsKeyPressed(KEY_P))
    {
        if (camera.projection == CAMERA_PERSPECTIVE)
        {
            // Create isometric view
            cameraMode = CAMERA_THIRD_PERSON;
            // Note: The target distance is related to the render distance in the orthographic projection
            camera.position = (Vector3){ 0.0f, 2.0f, -100.0f };
            camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
            camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
            camera.projection = CAMERA_ORTHOGRAPHIC;
            camera.fovy = 20.0f; // near plane width in CAMERA_ORTHOGRAPHIC
            CameraYaw(&camera, -135 * DEG2RAD, true);
            CameraPitch(&camera, -45 * DEG2RAD, true, true, false);
        }
        else if (camera.projection == CAMERA_ORTHOGRAPHIC)
        {
            // Reset to default view
            cameraMode = CAMERA_THIRD_PERSON;
            camera.position = (Vector3){ 0.0f, 2.0f, 10.0f };
            camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
            camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
            camera.projection = CAMERA_PERSPECTIVE;
            camera.fovy = 60.0f;
        }
    }


    // Toggle debug info with F5
    if (IsKeyPressed(KEY_FIVE)) {
        m_showDebug = !m_showDebug;
    }


}

