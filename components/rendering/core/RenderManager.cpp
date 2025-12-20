#include "RenderManager.h"
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>


RenderManager::RenderManager()
{
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

bool RenderManager::Initialize(int width, int height, const char *title)
{
    TraceLog(LOG_INFO, "Initializing RenderManager...");

    m_screenWidth = width;
    m_screenHeight = height;

    // Initialize window
    InitWindow(width, height, title);
    SetTargetFPS(60);

    // Initialize ImGui
    rlImGuiSetup(true);

    // Setup default camera
    m_camera.position = Vector3{0.0f, 10.0f, 10.0f};
    m_camera.target = Vector3{0.0f, 0.0f, 0.0f};
    m_camera.up = Vector3{0.0f, 1.0f, 0.0f};
    m_camera.fovy = 60.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;

    m_initialized = true;
    TraceLog(LOG_INFO, "RenderManager initialized successfully");
    return true;
}

void RenderManager::Shutdown()
{
    TraceLog(LOG_INFO, "Shutting down RenderManager...");

    if (m_font.texture.id != 0 && m_font.texture.id != GetFontDefault().texture.id)
    {
        UnloadFont(m_font);
    }

    rlImGuiShutdown();
    CloseWindow();

    m_initialized = false;
    TraceLog(LOG_INFO, "RenderManager shutdown complete");
}

void RenderManager::Update(float deltaTime)
{
    // Update logic if needed
}

void RenderManager::BeginFrame()
{
    BeginDrawing();
    ClearBackground(m_backgroundColor);

    // Build ImGui fonts on first frame
    static bool fontsBuilt = false;
    if (!fontsBuilt)
    {
        ImGuiIO &io = ImGui::GetIO();
        if (!io.Fonts->IsBuilt())
        {
            io.Fonts->Build();
            fontsBuilt = true;
        }
    }
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




