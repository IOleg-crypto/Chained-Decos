#include "Window.h"
#include <iostream>
#include <raylib.h>

namespace ChainedEngine
{

Window::Window(const WindowProps &props)
{
    Init(props);
}

Window::~Window()
{
    Shutdown();
}

void Window::Init(const WindowProps &props)
{
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;
    m_Data.Fullscreen = props.Fullscreen;
    m_Data.VSync = props.VSync;

    // Raylib Initialization
    // If we want logging, we assume it's set up elsewhere or we configure it here
    // SetTraceLogLevel(LOG_INFO);

    if (m_Data.Fullscreen)
    {
        // Monitor selection logic could go here
        // For now, simple InitWindow
        // Note: Raylib ToggleFullscreen logic is usually after InitWindow
    }

    InitWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str());

    if (m_Data.Fullscreen)
    {
        // If we want to start in fullscreen
        if (!IsWindowFullscreen())
        {
            // This might resize the window to monitor size
            ToggleFullscreen();
        }
    }

    if (m_Data.VSync)
    {
        SetTargetFPS(60); // Simple VSync simulation often maps to 60 or Monitor Refresh Rate
    }
    else
    {
        SetTargetFPS(0); // Unlimited
    }

    // Default config flags
    SetWindowState(FLAG_WINDOW_RESIZABLE);
}

void Window::Shutdown()
{
    CloseWindow();
}

void Window::OnUpdate()
{
    // If we ever need to handle window events manually
}

int Window::GetWidth() const
{
    return GetScreenWidth(); // Raylib global or m_Data.Width
}

int Window::GetHeight() const
{
    return GetScreenHeight(); // Raylib global or m_Data.Height
}

void Window::SetVSync(bool enabled)
{
    m_Data.VSync = enabled;
    if (enabled)
        SetTargetFPS(60);
    else
        SetTargetFPS(0);
}

bool Window::IsVSync() const
{
    return m_Data.VSync;
}

void Window::SetTitle(const std::string &title)
{
    m_Data.Title = title;
    SetWindowTitle(title.c_str());
}

void Window::SetIcon(const std::string &path)
{
    Image icon = LoadImage(path.c_str());
    if (icon.data != nullptr)
    {
        ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        SetWindowIcon(icon);
        UnloadImage(icon);
    }
}

bool Window::ShouldClose() const
{
    return WindowShouldClose();
}

} // namespace ChainedEngine
#include "core/Log.h"

