#include "RenderManager.h"
#include "core/Log.h"
#include <imgui.h>
#include <raylib.h>
#include <rlImGui.h>

namespace CHEngine
{
static std::unique_ptr<RenderManager> s_Instance = nullptr;

void RenderManager::Init(int width, int height, const char *title)
{
    s_Instance = std::make_unique<RenderManager>();
    s_Instance->InternalInitialize(width, height, title);
}

void RenderManager::Shutdown()
{
    if (s_Instance)
        s_Instance->InternalShutdown();
    s_Instance.reset();
}

bool RenderManager::IsInitialized()
{
    return s_Instance != nullptr;
}

void RenderManager::Update(float deltaTime)
{
    if (s_Instance)
        s_Instance->InternalUpdate(deltaTime);
}

void RenderManager::BeginFrame()
{
    if (s_Instance)
        s_Instance->InternalBeginFrame();
}

void RenderManager::EndFrame()
{
    if (s_Instance)
        s_Instance->InternalEndFrame();
}

void RenderManager::BeginMode3D(Camera camera)
{
    if (s_Instance)
        s_Instance->InternalBeginMode3D(camera);
}

void RenderManager::EndMode3D()
{
    if (s_Instance)
        s_Instance->InternalEndMode3D();
}

Camera &RenderManager::GetCamera()
{
    static Camera defaultCamera = {0};
    return s_Instance ? s_Instance->InternalGetCamera() : defaultCamera;
}

void RenderManager::SetCamera(const Camera &camera)
{
    if (s_Instance)
        s_Instance->InternalSetCamera(camera);
}

int RenderManager::GetScreenWidth()
{
    return s_Instance ? s_Instance->InternalGetScreenWidth() : 0;
}

int RenderManager::GetScreenHeight()
{
    return s_Instance ? s_Instance->InternalGetScreenHeight() : 0;
}

void RenderManager::SetBackgroundColor(Color color)
{
    if (s_Instance)
        s_Instance->InternalSetBackgroundColor(color);
}

Color RenderManager::GetBackgroundColor()
{
    return s_Instance ? s_Instance->InternalGetBackgroundColor() : BLACK;
}

void RenderManager::ToggleDebugInfo()
{
    if (s_Instance)
        s_Instance->InternalToggleDebugInfo();
}

void RenderManager::SetDebugInfo(bool enabled)
{
    if (s_Instance)
        s_Instance->InternalSetDebugInfo(enabled);
}

bool RenderManager::IsDebugInfoVisible()
{
    return s_Instance ? s_Instance->InternalIsDebugInfoVisible() : false;
}

bool RenderManager::IsCollisionDebugVisible()
{
    return s_Instance ? s_Instance->InternalIsCollisionDebugVisible() : false;
}

void RenderManager::SetCollisionDebugVisible(bool visible)
{
    if (s_Instance)
        s_Instance->InternalSetCollisionDebugVisible(visible);
}

Font RenderManager::GetFont()
{
    static Font defaultFont = {0};
    return s_Instance ? s_Instance->InternalGetFont() : defaultFont;
}

void RenderManager::SetFont(Font font)
{
    if (s_Instance)
        s_Instance->InternalSetFont(font);
}
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

bool RenderManager::InternalInitialize(int width, int height, const char *title)
{
    CD_CORE_INFO("Initializing RenderManager...");

    m_screenWidth = width;
    m_screenHeight = height;

    // Initialize window
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

void RenderManager::InternalShutdown()
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

void RenderManager::InternalUpdate(float deltaTime)
{
    // Update logic if needed
}

void RenderManager::InternalBeginFrame()
{
    BeginDrawing();
    ClearBackground(m_backgroundColor);
}

void RenderManager::InternalEndFrame()
{
    EndDrawing();
}

void RenderManager::InternalBeginMode3D(Camera camera)
{
    ::BeginMode3D(camera);
}

void RenderManager::InternalEndMode3D()
{
    ::EndMode3D();
}

int RenderManager::InternalGetScreenWidth() const
{
    return m_screenWidth;
}

int RenderManager::InternalGetScreenHeight() const
{
    return m_screenHeight;
}
} // namespace CHEngine
