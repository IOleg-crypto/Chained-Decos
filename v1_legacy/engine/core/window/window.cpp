#include "window.h"
#include "core/log.h"
#include "core/utils/base.h"
#include "events/application_event.h"
#include <glad.h>
#include <raylib.h>

namespace CHEngine
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

    InitWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str());

    CD_CORE_INFO("OpenGL Vendor:   %s", glGetString(GL_VENDOR));
    CD_CORE_INFO("OpenGL Renderer: %s", glGetString(GL_RENDERER));
    CD_CORE_INFO("OpenGL Version:  %s", glGetString(GL_VERSION));

    SetExitKey(0);

    if (m_Data.Fullscreen && !IsWindowFullscreen())
        ToggleFullscreen();

    SetTargetFPS(m_Data.VSync ? 60 : 0);
    SetWindowState(FLAG_WINDOW_RESIZABLE);
}

void Window::Shutdown()
{
    CloseWindow();
}

void Window::OnUpdate()
{
    if (WindowShouldClose())
    {
        WindowCloseEvent event;
        m_Data.EventCallback(event);
    }

    if (IsWindowResized())
    {
        m_Data.Width = GetScreenWidth();
        m_Data.Height = GetScreenHeight();
        WindowResizeEvent event(m_Data.Width, m_Data.Height);
        m_Data.EventCallback(event);
    }
}

int Window::GetWidth() const
{
    return GetScreenWidth();
}
int Window::GetHeight() const
{
    return GetScreenHeight();
}

void Window::SetVSync(bool enabled)
{
    m_Data.VSync = enabled;
    SetTargetFPS(enabled ? 60 : 0);
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
} // namespace CHEngine
