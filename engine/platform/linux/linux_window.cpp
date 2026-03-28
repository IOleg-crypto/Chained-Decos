#include "linux_window.h"
#include "engine/core/log.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace CHEngine
{

static void GLFWErrorCallback(int error, const char* description)
{
    CH_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

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

    if (!glfwInit())
    {
        CH_CORE_ASSERT(false, "Could not initialize GLFW!");
    }
    glfwSetErrorCallback(GLFWErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_WindowHandle = glfwCreateWindow((int)m_Width, (int)m_Height, m_Title.c_str(), nullptr, nullptr);
    CH_CORE_ASSERT(m_WindowHandle, "Failed to create GLFW window!");

    glfwMakeContextCurrent(m_WindowHandle);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    SetVSync(m_VSync);
}

void LinuxWindow::Shutdown()
{
    if (m_WindowHandle)
    {
        glfwDestroyWindow(m_WindowHandle);
    }
    CH_CORE_INFO("Linux Window Closed");
}

void LinuxWindow::BeginFrame()
{
}

void LinuxWindow::EndFrame()
{
    glfwSwapBuffers(m_WindowHandle);
    glfwPollEvents();
}

bool LinuxWindow::ShouldClose() const
{
    return glfwWindowShouldClose(m_WindowHandle);
}

void LinuxWindow::SetTitle(const std::string& title)
{
    m_Title = title;
    glfwSetWindowTitle(m_WindowHandle, m_Title.c_str());
}

void LinuxWindow::SetSize(int width, int height)
{
    m_Width = width;
    m_Height = height;
    glfwSetWindowSize(m_WindowHandle, m_Width, m_Height);
}

void LinuxWindow::SetSizeDirect(int width, int height)
{
    m_Width = width;
    m_Height = height;
}

void LinuxWindow::ToggleFullscreen()
{
    // Similar to windows implementation
}

void LinuxWindow::SetFullscreen(bool enabled)
{
}

void LinuxWindow::SetVSync(bool enabled)
{
    m_VSync = enabled;
    glfwSwapInterval(m_VSync ? 1 : 0);
}

void LinuxWindow::SetAntialiasing(bool enabled)
{
}

void LinuxWindow::SetTargetFramesPerSecond(int framesPerSecond)
{
}

} // namespace CHEngine
