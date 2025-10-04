//
// RenderManager.cpp - Implementation of rendering system
//

#include "RenderManager.h"
#include <Collision/CollisionDebugRenderer.h>
#include <Collision/CollisionManager.h>
#include <Engine/World/World.h>
#include <Game/Menu/Menu.h>
#include <Game/Player/Player.h>
#include <Model/Model.h>
#include <Physics/PhysicsComponent.h>
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>
#include <fstream>


// ==================== CONSTANTS ====================

RenderManager::RenderManager()
    : m_collisionDebugRenderer(std::make_unique<CollisionDebugRenderer>())
{
    m_font = {0};
    TraceLog(LOG_INFO, "RenderManager created");
}

RenderManager::~RenderManager()
{
    if (m_font.texture.id != 0 && m_font.texture.id != GetFontDefault().texture.id)
    {
        UnloadFont(m_font);
        TraceLog(LOG_INFO, "Custom font unloaded");
    }
    TraceLog(LOG_INFO, "RenderManager destroyed");
}

void RenderManager::Initialize()
{
    TraceLog(LOG_INFO, "Initializing render manager...");

    // Initialize ImGui
    rlImGuiSetup(true);

    // Use Alan Sans font for the entire game UI
    const std::string alanSansFontPath = PROJECT_ROOT_DIR "/resources/font/AlanSans.ttf";
    float fontSize = 18.0f; // Slightly larger for better readability

    // Initialize ImGui font first
    InitializeImGuiFont(alanSansFontPath, fontSize);

    // Init default font for raylib with higher resolution for smoother rendering
    if (FileExists(alanSansFontPath.c_str()))
    {
        m_font = LoadFontEx(alanSansFontPath.c_str(), 128, nullptr, 0);
        if (m_font.texture.id != 0)
        {
            // Set texture filter to linear for smoother scaling
            SetTextureFilter(m_font.texture, TEXTURE_FILTER_BILINEAR);
            TraceLog(LOG_INFO, "Alan Sans font loaded successfully with smooth filtering: %s",
                     alanSansFontPath.c_str());
        }
        else
        {
            TraceLog(LOG_WARNING, "Failed to load Alan Sans font for raylib: %s, using default font",
                     alanSansFontPath.c_str());
            m_font = GetFontDefault();
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "Alan Sans font file not found for raylib: %s, using default font",
                 alanSansFontPath.c_str());
        m_font = GetFontDefault();
    }

    TraceLog(LOG_INFO, "Render manager initialized successfully");
}


void RenderManager::InitializeImGuiFont(const std::string &fontPath, float fontSize)
{
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();

    // Check if font file exists first
    if (FileExists(fontPath.c_str()))
    {
        // Try to load Alan Sans font for ImGui
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize);
        if (font != nullptr)
        {
            io.Fonts->Build();
            TraceLog(LOG_INFO, "Alan Sans font loaded for ImGui: %s (%.1fpx)", fontPath.c_str(), fontSize);
            return;
        }
        else
        {
            TraceLog(LOG_WARNING, "Failed to load Alan Sans font for ImGui: %s, using default ImGui font", fontPath.c_str());
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "Alan Sans font file not found: %s, using default ImGui font", fontPath.c_str());
    }

    // Add default font as fallback
    io.Fonts->AddFontDefault();
    io.Fonts->Build();
}

void RenderManager::BeginFrame() const
{
    BeginDrawing();
    ClearBackground(m_backgroundColor);
}

void RenderManager::EndFrame() { EndDrawing(); }

void RenderManager::RenderGame(const Player &player, const ModelLoader &models,
                               const CollisionManager &collisionManager, bool showCollisionDebug)
{
    // Begin 3D rendering
    BeginMode3D(player.GetCameraController()->GetCamera());

    // Draw 3D scene
    DrawScene3D(models);
    DrawPlayer(player, models);

    // Update player collision for next frame
    player.UpdatePlayerCollision();

    // Draw collision debug if enabled
    if (showCollisionDebug)
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

void RenderManager::RenderDebugInfo(const Player &player, const ModelLoader &models,
                                    const CollisionManager &collisionManager)
{
    if (m_showDebugInfo)
    {
        DrawDebugInfoWindow(player, models, collisionManager);
    }
}

void RenderManager::BeginMode3D(const Camera &camera) { ::BeginMode3D(camera); }

void RenderManager::EndMode3D() { ::EndMode3D(); }

void RenderManager::DrawScene3D(const ModelLoader &models)
{
    // Draw ground plane for visual reference
    const Vector3 groundCenter = {0.0f, PhysicsComponent::WORLD_FLOOR_Y + 1.0f, 0.0f};
    const Vector2 groundSize = {2000.0f, 2000.0f};
    DrawPlane(groundCenter, groundSize, LIGHTGRAY);

    // Draw all models
    models.DrawAllModels();
}

void RenderManager::DrawPlayer(const Player &player, const ModelLoader &models)
{
    // Get player model from models cache
    Model &playerModel = const_cast<ModelLoader &>(models).GetModelByName("player");

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
                                          const Player &player) const
{
    // Draw small debug cube at player position for reference (less intrusive)
    const Vector3 playerPos = player.GetPlayerPosition();
    const Vector3 dbgPos = {playerPos.x, playerPos.y - 2.0f, playerPos.z}; // Below player
    DrawCubeWires(dbgPos, 0.5f, 0.5f, 0.5f, YELLOW);

    if (!m_collisionDebugRenderer)
        return;

    const auto &colliders = collisionManager.GetColliders();

    // Render all collisions
    m_collisionDebugRenderer->RenderAllCollisions(colliders);

    // Render player collision
    m_collisionDebugRenderer->RenderPlayerCollision(player.GetCollision());

    // Extra: show triangle counts for quick sanity (moved to bottom-right to avoid timer overlap)
    int y = GetScreenHeight() - 100; // Start from bottom
    int x = GetScreenWidth() - 150;  // Right side, away from timer
    int count = 0;
    for (const auto &c : colliders)
    {
        if (count >= 8) break; // Limit to 8 entries max
        DrawText(TextFormat("tri:%zu", c->GetTriangleCount()), x, y, 10, YELLOW);
        y += 12;
        count++;
    }

    TraceLog(LOG_DEBUG, "Collision debug rendered via CollisionDebugRenderer with %zu colliders",
             colliders.size());
}

void RenderManager::SetBackgroundColor(Color color) { m_backgroundColor = color; }

void RenderManager::ToggleDebugInfo() { m_showDebugInfo = !m_showDebugInfo; }

void RenderManager::DrawDebugInfoWindow(const Player &player, const ModelLoader &models,
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

void RenderManager::DrawModelManagerInfo(const ModelLoader &models)
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
        models.PrintStatistics();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cache Info"))
    {
        models.PrintCacheInfo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cleanup Cache"))
    {
        models.CleanupUnusedModels();
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
        if (collider->IsUsingOctree())
        {
            bvhColliders++;
            totalTriangles += collider->GetTriangleCount();
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
}

void RenderManager::DrawControlsInfo()
{
    ImGui::Text("Controls:");
    ImGui::Text("- F2: Toggle Debug Info");
    ImGui::Text("- F3: Toggle Collision Debug");
}

void RenderManager::ToggleCollisionDebug() { m_showCollisionDebug = !m_showCollisionDebug; }

void RenderManager::ForceCollisionDebugNextFrame() { m_forceCollisionDebugNextFrame = true; }

void RenderManager::SetDebugInfo(bool enabled) { m_showDebugInfo = enabled; }

void RenderManager::SetCollisionDebug(bool enabled) { m_showCollisionDebug = enabled; }

bool RenderManager::IsDebugInfoVisible() const { return m_showDebugInfo; }

bool RenderManager::IsCollisionDebugVisible() const { return m_showCollisionDebug; }

void RenderManager::ShowMetersPlayer(const Player &player) const
{
    Vector3 playerPosition = player.GetPlayerPosition();
    float groundLevel = PhysicsComponent::WORLD_FLOOR_Y;
    float heightAboveGround = playerPosition.y - groundLevel;

    static float maxHeight = 0.0f;
    if (heightAboveGround > maxHeight)
    {
        maxHeight = heightAboveGround;
    }

    std::string heightText = TextFormat("Height: %.1f m", heightAboveGround);
    std::string recordText = TextFormat("Record: %.1f m", maxHeight);

    Color textColor = WHITE;
    if (heightAboveGround == 500)
    {
        textColor = GOLD;
    }

    int textX = 20;
    int textY = 20;

    // Improved font rendering with better spacing for smoother text
    if (m_font.texture.id != 0)
    {
        // Use improved spacing (2.0f instead of 1.0f) for smoother text rendering
        DrawTextEx(m_font, heightText.c_str(), {(float)textX, (float)textY}, 24, 2.0f, textColor);
        DrawTextEx(m_font, recordText.c_str(), {(float)textX, (float)(textY + 30)}, 24, 2.0f,
                   textColor);
    }
    else
    {
        // Use default font with DrawTextEx for better rendering even in fallback
        const Font defaultFont = GetFontDefault();
        DrawTextEx(defaultFont, heightText.c_str(), {(float)textX, (float)textY}, 24, 2.0f,
                   textColor);
        DrawTextEx(defaultFont, recordText.c_str(), {(float)textX, (float)(textY + 30)}, 24, 2.0f,
                   textColor);
    }

    int circleX = textX + 200;
    int circleY = textY + 12;
    float circleRadius = 15.f;

    DrawCircleLines(circleX, circleY, circleRadius, WHITE);

    bool isPhysicsGrounded = player.GetPhysics().IsGrounded();
    bool isCloseToGround = (heightAboveGround <= 0.5f && heightAboveGround >= -0.1f);

    bool isOnGround = isPhysicsGrounded || isCloseToGround;

    if (isOnGround)
    {
        Color groundColor = isPhysicsGrounded ? GREEN : YELLOW;
        DrawCircle(circleX, circleY, circleRadius - 2, groundColor);

        // Use DrawTextEx for status labels as well for consistency and smoothness
        Font fontToUse = (m_font.texture.id != 0) ? m_font : GetFontDefault();
        if (isPhysicsGrounded)
        {
            DrawTextEx(fontToUse, "GROUND",
                       {static_cast<float>(circleX - 25), static_cast<float>(circleY + 25)}, 12,
                       1.5f, GREEN);
        }
        else
        {
            DrawTextEx(fontToUse, "NEAR",
                       {static_cast<float>(circleX - 20), static_cast<float>(circleY + 25)}, 12,
                       1.5f, YELLOW);
        }
    }
    else
    {
        const Font fontToUse = (m_font.texture.id != 0) ? m_font : GetFontDefault();
        DrawTextEx(fontToUse, "AIR", {(float)(circleX - 15), (float)(circleY + 25)}, 12, 1.5f,
                   LIGHTGRAY);
    }
}

Font RenderManager::GetFont() const { return m_font; }

void RenderManager::Render() {}
