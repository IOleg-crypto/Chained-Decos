#include "windows_window.h"
#include "engine/core/log.h"
#include "engine/core/ch_assert.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace CHEngine
{

static void GLFWErrorCallback(int error, const char* description)
{
    CH_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

std::unique_ptr<Window> Window::Create(const WindowProperties& properties)
{
    return std::make_unique<WindowsWindow>(properties);
}

WindowsWindow::WindowsWindow(const WindowProperties& properties)
{
    Init(properties);
}

WindowsWindow::~WindowsWindow()
{
    Shutdown();
}

void WindowsWindow::Init(const WindowProperties& properties)
{
    m_Width = properties.Width;
    m_Height = properties.Height;
    m_Title = properties.Title;
    m_VSync = properties.VSync;

    CH_CORE_INFO("Initializing Windows Window: {} ({}x{})", m_Title, m_Width, m_Height);

    static bool s_GLFWInitialized = false;
    if (!s_GLFWInitialized)
    {
        int success = glfwInit();
        CH_CORE_ASSERT(success, "Could not initialize GLFW!");
        glfwSetErrorCallback(GLFWErrorCallback);
        s_GLFWInitialized = true;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_WindowHandle = glfwCreateWindow((int)m_Width, (int)m_Height, m_Title.c_str(), nullptr, nullptr);
    CH_CORE_ASSERT(m_WindowHandle, "Failed to create GLFW window!");

    glfwMakeContextCurrent(m_WindowHandle);
    int status = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    CH_CORE_ASSERT(status, "Failed to initialize Glad!");

    CH_CORE_INFO("OpenGL Info:");
    CH_CORE_INFO("  Vendor: {0}", (const char*)glGetString(GL_VENDOR));
    CH_CORE_INFO("  Renderer: {0}", (const char*)glGetString(GL_RENDERER));
    CH_CORE_INFO("  Version: {0}", (const char*)glGetString(GL_VERSION));

    SetVSync(m_VSync);
}

void WindowsWindow::Shutdown()
{
    if (m_WindowHandle)
    {
        glfwDestroyWindow(m_WindowHandle);
    }
    CH_CORE_INFO("Windows Window Closed");
}

void WindowsWindow::BeginFrame()
{
    // No-op for now as Renderer handles clears
}

void WindowsWindow::EndFrame()
{
    glfwSwapBuffers(m_WindowHandle);
    glfwPollEvents();
}

bool WindowsWindow::ShouldClose() const
{
    return glfwWindowShouldClose(m_WindowHandle);
}

void WindowsWindow::SetTitle(const std::string& title)
{
    m_Title = title;
    glfwSetWindowTitle(m_WindowHandle, m_Title.c_str());
}

void WindowsWindow::SetSize(int width, int height)
{
    m_Width = width;
    m_Height = height;
    glfwSetWindowSize(m_WindowHandle, m_Width, m_Height);
}

void WindowsWindow::SetSizeDirect(int width, int height)
{
    m_Width = width;
    m_Height = height;
}

void WindowsWindow::ToggleFullscreen()
{
    // Simplified fullscreen toggle
    static bool isFullscreen = false;
    static int windowedX, windowedY, windowedW, windowedH;

    if (!isFullscreen)
    {
        glfwGetWindowPos(m_WindowHandle, &windowedX, &windowedY);
        glfwGetWindowSize(m_WindowHandle, &windowedW, &windowedH);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_WindowHandle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        isFullscreen = true;
    }
    else
    {
        glfwSetWindowMonitor(m_WindowHandle, nullptr, windowedX, windowedY, windowedW, windowedH, 0);
        isFullscreen = false;
    }
}

void WindowsWindow::SetFullscreen(bool enabled)
{
    // Logic similar to ToggleFullscreen
}

void WindowsWindow::SetVSync(bool enabled)
{
    m_VSync = enabled;
    if (m_VSync)
        glfwSwapInterval(1);
    else
        glfwSwapInterval(0);
}

void WindowsWindow::SetAntialiasing(bool enabled)
{
    // MSAA is usually set via window hints before creation
}

void WindowsWindow::SetTargetFramesPerSecond(int framesPerSecond)
{
    // Not directly supported by GLFW, usually handled by custom timing loop
}

// void WindowsWindow::SetWindowIcon(const std::string& path)
// {
//     // Implementation with stbi_load or similar
// }

} // namespace CHEngine
