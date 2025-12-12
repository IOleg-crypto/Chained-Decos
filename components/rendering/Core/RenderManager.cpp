#include "RenderManager.h"
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

RenderManager::RenderManager()
    : m_camera{0}, m_backgroundColor(SKYBLUE), m_font{}, m_showDebugInfo(false),
      m_debugCollision(false), m_initialized(false), m_screenWidth(1280), m_screenHeight(720)
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

Camera &RenderManager::GetCamera()
{
    return m_camera;
}

const Camera &RenderManager::GetCamera() const
{
    return m_camera;
}

void RenderManager::SetCamera(const Camera &camera)
{
    m_camera = camera;
}

int RenderManager::GetScreenWidth() const
{
    return m_screenWidth;
}

int RenderManager::GetScreenHeight() const
{
    return m_screenHeight;
}

void RenderManager::SetBackgroundColor(Color color)
{
    m_backgroundColor = color;
}

Color RenderManager::GetBackgroundColor() const
{
    return m_backgroundColor;
}

void RenderManager::ToggleDebugInfo()
{
    m_showDebugInfo = !m_showDebugInfo;
}

void RenderManager::SetDebugInfo(bool enabled)
{
    m_showDebugInfo = enabled;
}

bool RenderManager::IsDebugInfoVisible() const
{
    return m_showDebugInfo;
}

bool RenderManager::IsCollisionDebugVisible() const
{
    return m_debugCollision;
}

void RenderManager::SetCollisionDebugVisible(bool visible)
{
    m_debugCollision = visible;
}

Font RenderManager::GetFont() const
{
    return m_font;
}

void RenderManager::SetFont(Font font)
{
    m_font = font;
}
