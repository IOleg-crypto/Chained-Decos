#include "RenderManager.h"
#include "core/Log.h"
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

RenderManager::RenderManager()
{
    CD_CORE_INFO("RenderManager created");
}

RenderManager::~RenderManager()
{
    if (m_font.texture.id != 0 && m_font.texture.id != GetFontDefault().texture.id)
    {
        UnloadFont(m_font);
        CD_CORE_INFO("Custom font unloaded");
    }
    CD_CORE_INFO("RenderManager destroyed");
}

bool RenderManager::Initialize(int width, int height, const char *title)
{
    CD_CORE_INFO("Initializing RenderManager...");

    m_screenWidth = width;
    m_screenHeight = height;

    // Initialize window
    // Initialize window
    // InitWindow(width, height, title); // MOVED TO CORE ENGINE
    SetTargetFPS(60);

    // Setup default camera
    m_camera.position = Vector3{0.0f, 10.0f, 10.0f};
    m_camera.target = Vector3{0.0f, 0.0f, 0.0f};
    m_camera.up = Vector3{0.0f, 1.0f, 0.0f};
    m_camera.fovy = 60.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;

    m_initialized = true;
    CD_CORE_INFO("RenderManager initialized successfully");
    return true;
}

void RenderManager::Shutdown()
{
    CD_CORE_INFO("Shutting down RenderManager...");

    if (m_font.texture.id != 0 && m_font.texture.id != GetFontDefault().texture.id)
    {
        UnloadFont(m_font);
    }

    // CloseWindow(); // MOVED TO CORE ENGINE

    m_initialized = false;
    CD_CORE_INFO("RenderManager shutdown complete");
}

void RenderManager::Update(float deltaTime)
{
    // Update logic if needed
}

void RenderManager::BeginFrame()
{
    BeginDrawing();
    ClearBackground(m_backgroundColor);
}

void RenderManager::EndFrame()
{
    EndDrawing();
}

void RenderManager::BeginMode3D(Camera camera)
{
    ::BeginMode3D(camera);
}

void RenderManager::EndMode3D()
{
    ::EndMode3D();
}

int RenderManager::GetScreenWidth() const
{
    return m_screenWidth;
}

int RenderManager::GetScreenHeight() const
{
    return m_screenHeight;
}
