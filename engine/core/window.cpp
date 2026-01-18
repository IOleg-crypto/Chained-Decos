#include "window.h"
#include "engine/core/log.h"
#include <rlImGui.h>

namespace CHEngine
{
Window::Window(const WindowConfig &config)
    : m_Width(config.Width), m_Height(config.Height), m_Title(config.Title)
{
    unsigned int flags = FLAG_MSAA_4X_HINT;
    if (config.Resizable)
        flags |= FLAG_WINDOW_RESIZABLE;
    if (config.Fullscreen)
        flags |= FLAG_FULLSCREEN_MODE;

    SetConfigFlags(flags);
    InitWindow(m_Width, m_Height, m_Title.c_str());

    if (config.VSync)
        SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
    else
        SetTargetFPS(config.TargetFPS);

    rlImGuiSetup(true);
    SetExitKey(KEY_NULL); // Prevent ESC from closing the app
    CH_CORE_INFO("Window Initialized: {} ({}x{})", m_Title, m_Width, m_Height);
}

Window::~Window()
{
    rlImGuiShutdown();
    CloseWindow();
    CH_CORE_INFO("Window Closed");
}

void Window::PollEvents()
{
    // Raylib polls events internally
}

bool Window::ShouldClose() const
{
    return WindowShouldClose();
}

void Window::BeginFrame()
{
    BeginDrawing();
    ClearBackground(DARKGRAY);
}

void Window::EndFrame()
{
    EndDrawing();
}

void Window::SetTitle(const std::string &title)
{
    m_Title = title;
    ::SetWindowTitle(m_Title.c_str());
}

void Window::ToggleFullscreen()
{
    ::ToggleFullscreen();
}

} // namespace CHEngine
