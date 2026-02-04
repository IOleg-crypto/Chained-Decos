#include "window.h"
#include "engine/core/log.h"
#include "raylib.h"

// ImGui
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

// GLFW for getting native handle
#ifndef GLFW_INCLUDE_NONE
    #define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

namespace CHEngine
{
Window::Window(const WindowProps &config)
    : m_Width(config.Width), m_Height(config.Height), m_Title(config.Title), m_VSync(config.VSync), m_IniFilename(config.IniFilename)
{
    CH_CORE_INFO("Initializing Window: {} ({}x{})", m_Title, m_Width, m_Height);

    // Use Raylib to create the window (it manages GLFW internally)
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

    // Get the actual GLFW window handle.
    m_Window = glfwGetCurrentContext();

    CH_CORE_INFO("GLFW Window Handle obtained: {}", (void *)m_Window);

    if (!m_Window)
    {
        CH_CORE_ERROR("Failed to get GLFW window handle! Is GLFW initialized?");
        return;
    }

    SetExitKey(KEY_NULL); // Prevent ESC from closing the app

    CH_CORE_INFO("Window Initialized: {} ({}x{})", m_Title, m_Width, m_Height);
}

Window::~Window()
{
    CloseWindow();
    m_Window = nullptr;
    CH_CORE_INFO("Window Closed");
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

void Window::SetVSync(bool enabled)
{
    m_VSync = enabled;
    if (m_VSync)
        ::SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
}

void Window::SetTargetFPS(int fps)
{
    if (!m_VSync)
        ::SetTargetFPS(fps);
}

void Window::SetWindowIcon(Image icon)
{
    if (m_Window)
        ::SetWindowIcon(icon);
}


} // namespace CHEngine
