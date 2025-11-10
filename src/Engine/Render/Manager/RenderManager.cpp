//
// RenderManager.cpp - Implementation of rendering system
//

#include "RenderManager.h"
#include "../Helpers/ImGuiHelper.h"
#include "../Interfaces/IGameRenderable.h"
#include <Collision/Debug/CollisionDebugRenderer.h>
#include <Collision/Manager/CollisionManager.h>
#include <Engine/World/Core/World.h>
#include <Model/Core/Model.h>
#include <Physics/Components/PhysicsComponent.h>
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>
#include <fstream>
#include <filesystem>


// ==================== CONSTANTS ====================

RenderManager::RenderManager()
    : m_collisionDebugRenderer(std::make_unique<CollisionDebugRenderer>()),
      m_shaderManager(std::make_unique<ShaderManager>())
{
    m_collisionDebugRenderer->SetWireframeMode(true); // Use wireframe for collision shapes in normal gameplay
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

bool RenderManager::Initialize()
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

    // Load player wind effect shader
    LoadWindShader();

    TraceLog(LOG_INFO, "Render manager initialized successfully");
    return true;
}


void RenderManager::InitializeImGuiFont(const std::string &fontPath, float fontSize)
{
    ImGuiHelper::InitializeFont(fontPath, fontSize);
}

void RenderManager::BeginFrame() const
{
    BeginDrawing();
    ClearBackground(m_backgroundColor);
    
    // Build ImGui fonts on first frame to ensure proper OpenGL context
    // Must be called after BeginDrawing() but before rlImGuiBegin()
    static bool fontsBuilt = false;
    if (!fontsBuilt) {
        ImGuiIO &io = ImGui::GetIO();
        if (!io.Fonts->IsBuilt()) {
            io.Fonts->Build();
            fontsBuilt = true;
        }
    }
}

void RenderManager::EndFrame() { EndDrawing(); }

void RenderManager::RenderGame(IGameRenderable &renderable, const ModelLoader &models,
                                 const CollisionManager &collisionManager, bool showCollisionDebug)
{
    // NOTE: BeginMode3D/EndMode3D are now handled by Game::RenderGameWorld()
    // to allow RenderEditorMap() to be called inside the 3D context

    // Draw 3D scene
    DrawScene3D(models);
    DrawPlayer(renderable, models);

    // Update renderable collision for next frame
    renderable.UpdateCollision();

    // Only render collision shapes in debug mode to avoid wireframes covering primitives
    // In normal gameplay, collision shapes are not visible (only functional)
    if (showCollisionDebug)
    {
        RenderCollisionShapes(collisionManager, renderable);
        RenderCollisionDebug(collisionManager, renderable);
        m_forceCollisionDebugNextFrame = false;
    }
}

void RenderManager::RenderMenu(IMenuRenderable &renderable)
{
    renderable.Update(); // Menu update doesn't need CollisionManager
    renderable.Render();
}

void RenderManager::RenderDebugInfo(IGameRenderable &renderable, const ModelLoader &models,
                                     const CollisionManager &collisionManager)
{
    if (m_showDebugInfo)
    {
        DrawDebugInfoWindow(renderable, models, collisionManager);
    }
}

void RenderManager::BeginMode3D(const Camera &camera) { ::BeginMode3D(camera); }

void RenderManager::EndMode3D() { ::EndMode3D(); }

void RenderManager::DrawScene3D(const ModelLoader &models)
{
    // Draw all models
    models.DrawAllModels();
}

// Player constants (defined locally to avoid include issues)
constexpr float MODEL_Y_OFFSET = -1.0f;
constexpr float MODEL_SCALE = 1.1f;

void RenderManager::DrawPlayer(IGameRenderable &renderable, const ModelLoader &models)
{
    // Get player model from models cache
    auto playerModelOpt = const_cast<ModelLoader &>(models).GetModelByName("player_low");
    if (!playerModelOpt) {
        TraceLog(LOG_ERROR, "RenderManager::DrawPlayer() - Player model not found!");
        // Draw a simple cube as fallback
        Vector3 pos = renderable.GetPosition();
        pos.y += MODEL_Y_OFFSET;
        DrawCube(pos, 1.0f, 2.0f, 1.0f, RED);
        DrawBoundingBox(renderable.GetBoundingBox(), GREEN);
        return;
    }

    Model& playerModel = playerModelOpt->get();
    if (playerModel.meshCount == 0) {
        TraceLog(LOG_ERROR, "RenderManager::DrawPlayer() - Player model has no meshes!");
        // Draw a simple cube as fallback
        Vector3 pos = renderable.GetPosition();
        pos.y += MODEL_Y_OFFSET;
        DrawCube(pos, 1.0f, 2.0f, 1.0f, RED);
        DrawBoundingBox(renderable.GetBoundingBox(), GREEN);
        return;
    }

    // Apply player rotation
    playerModel.transform = MatrixRotateY(DEG2RAD * renderable.GetRotationY());

    // Calculate adjusted position (with Y offset)
    Vector3 adjustedPos = renderable.GetPosition();
    adjustedPos.y += MODEL_Y_OFFSET;

    // Draw player model and bounding box
    DrawModel(playerModel, adjustedPos, MODEL_SCALE, WHITE);
    
    DrawBoundingBox(renderable.GetBoundingBox(), GREEN);
}

void RenderManager::RenderCollisionDebug(const CollisionManager &collisionManager,
                                           IGameRenderable &renderable) const
{
    TraceLog(LOG_DEBUG, "RenderManager::RenderCollisionDebug() - Starting collision debug rendering");

    // Draw small debug cube at player position for reference (less intrusive)
    const Vector3 playerPos = renderable.GetPosition();
    const Vector3 dbgPos = {playerPos.x, playerPos.y - 2.0f, playerPos.z}; // Below player
    DrawCubeWires(dbgPos, 0.5f, 0.5f, 0.5f, YELLOW);

    if (!m_collisionDebugRenderer)
    {
        TraceLog(LOG_WARNING, "RenderManager::RenderCollisionDebug() - No collision debug renderer available");
        return;
    }

    const auto &colliders = collisionManager.GetColliders();
    TraceLog(LOG_DEBUG, "RenderManager::RenderCollisionDebug() - Rendering %zu collision objects", colliders.size());

    // Render all collisions
    m_collisionDebugRenderer->RenderAllCollisions(colliders);

    // Render player collision
    m_collisionDebugRenderer->RenderPlayerCollision(renderable.GetCollision());

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

    TraceLog(LOG_DEBUG, "RenderManager::RenderCollisionDebug() - Collision debug rendered via CollisionDebugRenderer with %zu colliders",
              colliders.size());
}

void RenderManager::RenderCollisionShapes(const CollisionManager &collisionManager,
                                           IGameRenderable &renderable) const
{
    if (!m_collisionDebugRenderer)
    {
        TraceLog(LOG_WARNING, "RenderManager::RenderCollisionShapes() - No collision debug renderer available");
        return;
    }

    const auto &colliders = collisionManager.GetColliders();

    // Use wireframe mode for normal gameplay collision shapes
    m_collisionDebugRenderer->SetWireframeMode(true);

    // Render all collision shapes with subtle colors for normal gameplay
    for (size_t i = 0; i < colliders.size(); i++)
    {
        Color color = (i == 0) ? Fade(GREEN, 0.3f) : Fade(YELLOW, 0.3f); // Semi-transparent colors
        m_collisionDebugRenderer->RenderCollisionBox(*colliders[i].get(), color);
    }

    // Render player collision shape
    m_collisionDebugRenderer->RenderPlayerCollision(renderable.GetCollision());
}

void RenderManager::SetBackgroundColor(Color color) { m_backgroundColor = color; }

void RenderManager::ToggleDebugInfo() { m_showDebugInfo = !m_showDebugInfo; }

void RenderManager::DrawDebugInfoWindow(IGameRenderable &renderable, const ModelLoader &models,
                                         const CollisionManager &collisionManager)
{
    rlImGuiBegin();

    float window_width = 400.0f;
    float window_height = 350.0f;
    ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_Always);

    if (ImGui::Begin("Debug Info", nullptr, ImGuiWindowFlags_NoResize))
    {
        DrawCameraInfo(renderable.GetCamera(),
                       0); // For Menu, camera mode is not applicable, use 0

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

void RenderManager::ShowMetersPlayer(const IGameRenderable &renderable) const
{
    Vector3 playerPosition = renderable.GetPosition();
    
    // Don't show meters if player is at uninitialized position
    // Check for both default (0,0,0) and explicit uninitialized position (-999999)
    constexpr float UNINITIALIZED_POS = -999999.0f;
    if ((playerPosition.x == 0.0f && playerPosition.y == 0.0f && playerPosition.z == 0.0f) ||
        (playerPosition.x <= UNINITIALIZED_POS + 1000.0f && playerPosition.y <= UNINITIALIZED_POS + 1000.0f))
    {
        return;
    }
    
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

    bool isPhysicsGrounded = renderable.IsGrounded();
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

void RenderManager::Shutdown()
{
    TraceLog(LOG_INFO, "Shutting down render manager...");
    if (m_font.texture.id != 0 && m_font.texture.id != GetFontDefault().texture.id)
    {
        UnloadFont(m_font);
        TraceLog(LOG_INFO, "Custom font unloaded");
    }
    TraceLog(LOG_INFO, "Render manager shutdown complete");
}

bool RenderManager::LoadWindShader()
{
    // Unload existing shader if it exists (in case of reload)
    if (m_shaderManager->IsShaderLoaded("player_wind"))
    {
        m_shaderManager->UnloadShader("player_wind");
    }
    
    std::string vsPath = std::string(PROJECT_ROOT_DIR) + "/resources/shaders/player_effect.vs";
    std::string fsPath = std::string(PROJECT_ROOT_DIR) + "/resources/shaders/player_effect.fs";
    
    // Check if files exist
    if (!std::filesystem::exists(vsPath) || !std::filesystem::exists(fsPath))
    {
        TraceLog(LOG_WARNING, "Wind shader files not found: %s or %s", vsPath.c_str(), fsPath.c_str());
        return false;
    }
    
    // Only try to load if OpenGL context is ready
    if (!IsWindowReady())
    {
        TraceLog(LOG_DEBUG, "Wind shader loading deferred - OpenGL context not ready yet");
        return false;
    }
    
    if (m_shaderManager->LoadShaderPair("player_wind", vsPath, fsPath))
    {
        Shader* windShader = m_shaderManager->GetShader("player_wind");
        if (windShader && windShader->id != 0)
        {
            // Verify shader is valid using raylib's IsShaderValid
            if (IsShaderValid(*windShader))
            {
                m_fallSpeedLoc = GetShaderLocation(*windShader, "fallSpeed");
                m_timeLoc = GetShaderLocation(*windShader, "time");
                m_windDirectionLoc = GetShaderLocation(*windShader, "windDirection");
                
                TraceLog(LOG_INFO, "Player wind effect shader loaded successfully");
                TraceLog(LOG_INFO, "Shader locations: fallSpeed=%d, time=%d, windDirection=%d", 
                         m_fallSpeedLoc, m_timeLoc, m_windDirectionLoc);
                
                // Reset shader time when shader is loaded
                m_shaderTime = 0.0f;
                
                return true;
            }
            else
            {
                TraceLog(LOG_WARNING, "Wind shader loaded but IsShaderValid returned false");
                m_shaderManager->UnloadShader("player_wind");
                return false;
            }
        }
        else
        {
            TraceLog(LOG_WARNING, "Wind shader loaded but GetShader returned null or invalid ID");
            return false;
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "Failed to load player wind effect shader from %s + %s", vsPath.c_str(), fsPath.c_str());
        return false;
    }
}

void RenderManager::Update(float deltaTime)
{
    // No update needed for RenderManager
}
