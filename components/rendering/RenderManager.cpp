#include "RenderManager.h"

namespace Servers
{

RenderManager::~RenderManager()
{
    Shutdown();
}

bool RenderManager::Initialize(int width, int height, const std::string &title)
{
    m_width = width;
    m_height = height;
    return true;
}

void RenderManager::Shutdown()
{
    // Cleanup if needed
}

void RenderManager::BeginFrame()
{
    BeginDrawing();
    ClearBackground(SKYBLUE);
}

void RenderManager::EndFrame()
{
    if (m_debug)
    {
        DrawFPS(10, 10);
    }
    EndDrawing();
}

void RenderManager::BeginMode3D(Camera3D camera)
{
    ::BeginMode3D(camera);
}

void RenderManager::EndMode3D()
{
    ::EndMode3D();
}

int RenderManager::GetScreenWidth() const
{
    return m_width;
}

int RenderManager::GetScreenHeight() const
{
    return m_height;
}

void RenderManager::SetDebug(bool enabled)
{
    m_debug = enabled;
}

bool RenderManager::IsDebug() const
{
    return m_debug;
}

} // namespace Servers
