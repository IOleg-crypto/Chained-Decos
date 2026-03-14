#include "linux_window.h"
#include "engine/core/log.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace CHEngine
{

// On Linux, the factory would be in a separate file or guarded by macros.
// Since we are likely building for one platform at a time, we can use macros in a shared factory.

LinuxWindow::LinuxWindow(const WindowProperties& properties)
{
    Init(properties);
}

LinuxWindow::~LinuxWindow()
{
    Shutdown();
}

void LinuxWindow::Init(const WindowProperties& properties)
{
    m_Width = properties.Width;
    m_Height = properties.Height;
    m_Title = properties.Title;
    m_VSync = properties.VSync;

    CH_CORE_INFO("Initializing Linux Window: {} ({}x{})", m_Title, m_Width, m_Height);

    unsigned int flags = FLAG_MSAA_4X_HINT;
    if (properties.Resizable)
    {
        flags |= FLAG_WINDOW_RESIZABLE;
    }
    if (properties.Fullscreen)
    {
        flags |= FLAG_FULLSCREEN_MODE;
    }

    SetConfigFlags(flags);
    InitWindow(m_Width, m_Height, m_Title.c_str());

    if (m_VSync)
    {
        SetTargetFramesPerSecond(GetMonitorRefreshRate(GetCurrentMonitor()));
    }
    else
    {
        SetTargetFramesPerSecond(properties.TargetFramesPerSecond);
    }

    m_WindowHandle = glfwGetCurrentContext();
    SetExitKey(KEY_NULL);
}

void LinuxWindow::Shutdown()
{
    CloseWindow();
    CH_CORE_INFO("Linux Window Closed");
}

void LinuxWindow::BeginFrame()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);
}

void LinuxWindow::EndFrame()
{
    EndDrawing();
}

bool LinuxWindow::ShouldClose() const
{
    return WindowShouldClose();
}

void LinuxWindow::SetTitle(const std::string& title)
{
    m_Title = title;
    SetWindowTitle(m_Title.c_str());
}

void LinuxWindow::SetSize(int width, int height)
{
    m_Width = width;
    m_Height = height;
    SetWindowSize(m_Width, m_Height);
}

void LinuxWindow::SetSizeDirect(int width, int height)
{
    m_Width = width;
    m_Height = height;
}

void LinuxWindow::ToggleFullscreen()
{
    ::ToggleFullscreen();
}

void LinuxWindow::SetFullscreen(bool enabled)
{
    if (IsWindowFullscreen() != enabled)
    {
        ::ToggleFullscreen();
    }
}

void LinuxWindow::SetVSync(bool enabled)
{
    m_VSync = enabled;
    if (m_VSync)
    {
        SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
    }
}

void LinuxWindow::SetAntialiasing(bool enabled)
{
    if (enabled)
    {
        SetWindowState(FLAG_MSAA_4X_HINT);
    }
    else
    {
        ClearWindowState(FLAG_MSAA_4X_HINT);
    }
}

void LinuxWindow::SetTargetFramesPerSecond(int framesPerSecond)
{
    if (!m_VSync)
    {
        SetTargetFPS(framesPerSecond);
    }
}

void LinuxWindow::SetWindowIcon(Image icon)
{
    ::SetWindowIcon(icon);
}

} // namespace CHEngine
