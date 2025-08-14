//
// RenderManager.cpp - Implementation of rendering system
//

#include "RenderManager.h"
#include <Collision/CollisionDebugRenderer.h>
#include <Collision/CollisionManager.h>
#include <Menu/Menu.h>
#include <Model/Model.h>
#include <Player/Player.h>
#include <World/Physics.h>
#include <imgui.h>
#include <rcamera.h>
#include <rlImGui.h>

// ==================== CONSTANTS ====================

RenderManager::RenderManager()
    : m_collisionDebugRenderer(std::make_unique<CollisionDebugRenderer>()), m_showDebugInfo(false)
{
    TraceLog(LOG_INFO, "RenderManager created");
}

RenderManager::~RenderManager() { TraceLog(LOG_INFO, "RenderManager destroyed"); }

void RenderManager::Initialize()
{
    TraceLog(LOG_INFO, "Initializing render manager...");

    // Initialize ImGui
    rlImGuiSetup(true);

    // Use default font path - we'll make this configurable later
    const std::string defaultFontPath = PROJECT_ROOT_DIR "/resources/font/Lato/Lato-Black.ttf";
    float fontSize = 16.0f;
    InitializeImGuiFont(defaultFontPath, fontSize);

    TraceLog(LOG_INFO, "Render manager initialized successfully");
}

void RenderManager::InitializeImGuiFont(const std::string &fontPath, float fontSize)
{
    const ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
    io.Fonts->Build();
    TraceLog(LOG_INFO, "ImGui font loaded: %s (%.1fpx)", fontPath.c_str(), fontSize);
}

void RenderManager::BeginFrame()
{
    BeginDrawing();
    ClearBackground(m_backgroundColor);
}

void RenderManager::EndFrame() { EndDrawing(); }

void RenderManager::RenderGame(const Player &player, const Models &models,
                               const CollisionManager &collisionManager, bool showCollisionDebug)
{
    // Begin 3D rendering
    BeginMode3D(player.GetCameraController()->GetCamera());

    // Draw 3D scene
    DrawScene3D(models);
    DrawPlayer(player, models);

    // Update player collision for next frame
    const_cast<Player &>(player).UpdatePlayerCollision();

    // Draw collision debug if enabled
    if (showCollisionDebug || m_showCollisionDebug || m_forceCollisionDebugNextFrame)
    {
        RenderCollisionDebug(collisionManager, player);
        m_forceCollisionDebugNextFrame = false;
    }

    // End 3D rendering
    EndMode3D();
}

void RenderManager::RenderMenu(Menu &menu)
{
    menu.Update();
    menu.Render();
}

void RenderManager::RenderDebugInfo(const Player &player, const Models &models,
                                    const CollisionManager &collisionManager)
{
    if (m_showDebugInfo)
    {
        DrawDebugInfoWindow(player, models, collisionManager);
    }
}

void RenderManager::BeginMode3D(const Camera &camera) { ::BeginMode3D(camera); }

void RenderManager::EndMode3D() { ::EndMode3D(); }

void RenderManager::DrawScene3D(const Models &models)
{
    // Draw ground plane using constants from PhysicsComponent
    DrawPlane(PhysicsComponent::GROUND_POSITION, PhysicsComponent::GROUND_SIZE, LIGHTGRAY);

    // Draw all models
    models.DrawAllModels();
}

void RenderManager::DrawPlayer(const Player &player, const Models &models)
{
    // Get player model from models cache
    Model &playerModel = const_cast<Models &>(models).GetModelByName("player");

    // Apply player rotation
    playerModel.transform = MatrixRotateY(DEG2RAD * player.GetRotationY());

    // Calculate adjusted position (with Y offset)
    Vector3 adjustedPos = player.GetPlayerPosition();
    adjustedPos.y += Player::MODEL_Y_OFFSET;

    // Draw player model and bounding box
    DrawModel(playerModel, adjustedPos, Player::MODEL_SCALE, WHITE);
    DrawBoundingBox(player.GetPlayerBoundingBox(), GREEN);
}

void RenderManager::RenderCollisionDebug(const CollisionManager &collisionManager,
                                         const Player &player)
{
    // Draw debug cube using constants from PhysicsComponent
    DrawCubeWires(PhysicsComponent::DEBUG_CUBE_POSITION, PhysicsComponent::DEBUG_CUBE_SIZE.x,
                  PhysicsComponent::DEBUG_CUBE_SIZE.y, PhysicsComponent::DEBUG_CUBE_SIZE.z, YELLOW);

    if (m_collisionDebugRenderer)
    {
        const auto &colliders = collisionManager.GetColliders();
        m_collisionDebugRenderer->RenderAllCollisions(colliders);
        m_collisionDebugRenderer->RenderPlayerCollision(player.GetCollision());

        TraceLog(LOG_DEBUG, "Collision debug rendered via CollisionDebugRenderer");
    }
}

void RenderManager::SetBackgroundColor(Color color) { m_backgroundColor = color; }

void RenderManager::DrawDebugInfoWindow(const Player &player, const Models &models,
                                        const CollisionManager &collisionManager)
{
    rlImGuiBegin();

    float window_width = 400.0f;
    float window_height = 350.0f;
    ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_Always);

    if (ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_NoResize))
    {
        DrawCameraInfo(player.GetCameraController()->GetCamera(),
                       player.GetCameraController()->GetCameraMode());

        ImGui::Separator();
        DrawModelManagerInfo(models);

        ImGui::Separator();
        DrawCollisionSystemInfo(collisionManager);

        ImGui::Separator();
        DrawControlsInfo();
    }
    ImGui::End();

    rlImGuiEnd();
}

void RenderManager::DrawCameraInfo(const Camera &camera, int cameraMode)
{
    ImGui::Text("Camera Status:");

    const char *modeStr = (cameraMode == CAMERA_FREE)           ? "FREE"
                          : (cameraMode == CAMERA_FIRST_PERSON) ? "FIRST_PERSON"
                          : (cameraMode == CAMERA_THIRD_PERSON) ? "THIRD_PERSON"
                          : (cameraMode == CAMERA_ORBITAL)      ? "ORBITAL"
                                                                : "CUSTOM";
    ImGui::Text("- Mode: %s", modeStr);

    const char *projectionStr = (camera.projection == CAMERA_PERSPECTIVE)    ? "PERSPECTIVE"
                                : (camera.projection == CAMERA_ORTHOGRAPHIC) ? "ORTHOGRAPHIC"
                                                                             : "CUSTOM";
    ImGui::Text("- Projection: %s", projectionStr);

    ImGui::Text("- Position: (%.3f, %.3f, %.3f)", camera.position.x, camera.position.y,
                camera.position.z);
    ImGui::Text("- Target:   (%.3f, %.3f, %.3f)", camera.target.x, camera.target.y,
                camera.target.z);
    ImGui::Text("- Up:       (%.3f, %.3f, %.3f)", camera.up.x, camera.up.y, camera.up.z);
    ImGui::Text("FPS: %d", GetFPS());
}

void RenderManager::DrawModelManagerInfo(const Models &models)
{
    ImGui::Text("Model Manager:");

    const auto &stats = models.GetLoadingStats();
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
        const_cast<Models &>(models).PrintStatistics();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cache Info"))
    {
        const_cast<Models &>(models).PrintCacheInfo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cleanup Cache"))
    {
        const_cast<Models &>(models).CleanupUnusedModels();
    }
}

void RenderManager::DrawCollisionSystemInfo(const CollisionManager &collisionManager)
{
    ImGui::Text("Collision System:");

    const auto &colliders = collisionManager.GetColliders();
    size_t bvhColliders = 0;
    size_t meshColliders = 0;
    size_t totalTriangles = 0;

    for (const auto &collider : colliders)
    {
        if (collider.IsUsingOctree())
        {
            bvhColliders++;
            totalTriangles += collider.GetTriangleCount();
        }
        else
        {
            meshColliders++;
        }
    }

    ImGui::Text("- Total colliders: %zu", colliders.size());
    ImGui::Text("- BVH colliders: %zu", bvhColliders);
    ImGui::Text("- Mesh/AABB colliders: %zu", meshColliders);
    if (totalTriangles > 0)
    {
        ImGui::Text("- Total triangles in BVH: %zu", totalTriangles);
    }

    if (ImGui::Button("Test Collision Ray Cast (F12)"))
    {
        // This will need to trigger the test - we'll add callback later
        TraceLog(LOG_INFO, "Ray cast test requested from debug UI");
    }
}

void RenderManager::DrawControlsInfo()
{
    ImGui::Text("Controls:");
    ImGui::Text("- F2: Toggle Debug Info");
    ImGui::Text("- F3: Toggle Collision Debug");
    ImGui::Text("- F12: Test Collision Ray Casting");
}