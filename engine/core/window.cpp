#include "window.h"
#include "engine/core/log.h"
#include "raylib.h"
#include "engine/graphics/raylib_context.h"

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
Window::Window(const WindowProperties &windowProperties)
    : m_Width(windowProperties.Width), m_Height(windowProperties.Height), m_Title(windowProperties.Title), m_VSync(windowProperties.VSync), m_ImGuiConfigurationPath(windowProperties.ImGuiConfigurationPath)
{
    CH_CORE_INFO("Initializing Window: {} ({}x{})", m_Title, m_Width, m_Height);

    // Use Raylib to create the window (it manages GLFW internally)
    unsigned int flags = FLAG_MSAA_4X_HINT;
    if (windowProperties.Resizable)
        flags |= FLAG_WINDOW_RESIZABLE;
    if (windowProperties.Fullscreen)
        flags |= FLAG_FULLSCREEN_MODE;

    SetConfigFlags(flags);
    InitWindow(m_Width, m_Height, m_Title.c_str());

    if (windowProperties.VSync)
        SetTargetFramesPerSecond(GetMonitorRefreshRate(GetCurrentMonitor()));
    else
        SetTargetFramesPerSecond(windowProperties.TargetFramesPerSecond);

    m_WindowHandle = glfwGetCurrentContext();

    CH_CORE_INFO("GLFW Window Handle obtained: {}", (void *)m_WindowHandle);

    if (!m_WindowHandle)
    {
        CH_CORE_ERROR("Failed to get GLFW window handle! Is GLFW initialized?");
        return;
    }

    m_Context = std::make_unique<RaylibContext>(m_WindowHandle);
    m_Context->Init();

    SetExitKey(KEY_NULL); // Prevent ESC from closing the app

    CH_CORE_INFO("Window Initialized: {} ({}x{})", m_Title, m_Width, m_Height);
}

Window::~Window()
{
    CloseWindow();
    m_WindowHandle = nullptr;
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

void Window::SetTargetFramesPerSecond(int framesPerSecond)
{
    if (!m_VSync)
        ::SetTargetFPS(framesPerSecond);
}

void Window::SetWindowIcon(Image icon)
{
    if (m_WindowHandle)
        ::SetWindowIcon(icon);
}


} // namespace CHEngine
