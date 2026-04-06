#include "glfw_window.h"
#include "engine/core/log.h"
#include "engine/core/ch_assert.h"
#include "engine/core/input.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

namespace CHEngine
{

static void GLFWErrorCallback(int error, const char* description)
{
    CH_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

GlfwWindow::GlfwWindow(const WindowProperties& properties)
{
    Init(properties);
}

GlfwWindow::~GlfwWindow()
{
    Shutdown();
}

void GlfwWindow::Init(const WindowProperties& properties)
{
    int initialWidth = properties.Width;
    int initialHeight = properties.Height;
    m_Title = properties.Title;
    m_VSync = properties.VSync;
    m_TargetFPS = properties.TargetFramesPerSecond;

    static bool s_GLFWInitialized = false;
    if (!s_GLFWInitialized)
    {
        int success = glfwInit();
        CH_CORE_ASSERT(success, "Could not initialize GLFW!");
        glfwSetErrorCallback(GLFWErrorCallback);
        s_GLFWInitialized = true;
    }

    if (initialWidth <= 0 || initialHeight <= 0)
    {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (monitor)
        {
            int workX = 0, workY = 0, workW = 0, workH = 0;
            glfwGetMonitorWorkarea(monitor, &workX, &workY, &workW, &workH);
            initialWidth = (workW > 0) ? workW : 1280;
            initialHeight = (workH > 0) ? workH : 720;
        }
        else
        {
            initialWidth = 1280;
            initialHeight = 720;
        }
    }

    m_Width = initialWidth;
    m_Height = initialHeight;
    CH_CORE_INFO("Initializing Glfw Window: {} ({}x{})", m_Title, m_Width, m_Height);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

    m_WindowHandle = glfwCreateWindow((int)m_Width, (int)m_Height, m_Title.c_str(), nullptr, nullptr);
    CH_CORE_ASSERT(m_WindowHandle, "Failed to create GLFW window!");

    glfwMakeContextCurrent(m_WindowHandle);
    glfwSetWindowUserPointer(m_WindowHandle, this);

    // Initial framebuffer size (crucial for Retina/HiDPI)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_WindowHandle, &fbWidth, &fbHeight);
    m_Width = (uint32_t)fbWidth;
    m_Height = (uint32_t)fbHeight;

    // Resize Callback
    glfwSetFramebufferSizeCallback(m_WindowHandle, [](GLFWwindow* window, int width, int height) {
        auto& glWindow = *(GlfwWindow*)glfwGetWindowUserPointer(window);
        glWindow.SetSizeDirect(width, height);
        // Important: Viewport should be updated here or in the renderer
        glViewport(0, 0, width, height);
    });

    // Scroll Callback for mouse wheel input
    glfwSetScrollCallback(m_WindowHandle, [](GLFWwindow* window, double xOffset, double yOffset) {
        Input::OnMouseScroll((float)xOffset, (float)yOffset);
    });
    
    // Platform-neutral GLAD loading
    int status = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    CH_CORE_ASSERT(status, "Failed to initialize Glad!");

    CH_CORE_INFO("OpenGL Info:");
    CH_CORE_INFO("  Vendor: {0}", (const char*)glGetString(GL_VENDOR));
    CH_CORE_INFO("  Renderer: {0}", (const char*)glGetString(GL_RENDERER));
    CH_CORE_INFO("  Version: {0}", (const char*)glGetString(GL_VERSION));

    SetVSync(m_VSync);
}

void GlfwWindow::Shutdown()
{
    if (m_WindowHandle)
    {
        glfwDestroyWindow(m_WindowHandle);
    }
    CH_CORE_INFO("Glfw Window Closed");
}

void GlfwWindow::BeginFrame()
{
}

void GlfwWindow::EndFrame()
{
    glfwSwapBuffers(m_WindowHandle);
    glfwPollEvents();
}

bool GlfwWindow::ShouldClose() const
{
    return glfwWindowShouldClose(m_WindowHandle);
}

void GlfwWindow::SetTitle(const std::string& title)
{
    m_Title = title;
    glfwSetWindowTitle(m_WindowHandle, m_Title.c_str());
}

void GlfwWindow::SetSize(int width, int height)
{
    m_Width = width;
    m_Height = height;
    glfwSetWindowSize(m_WindowHandle, m_Width, m_Height);
}

void GlfwWindow::SetSizeDirect(int width, int height)
{
    m_Width = width;
    m_Height = height;
}

void GlfwWindow::ToggleFullscreen()
{
    SetFullscreen(!m_IsFullscreen);
}

void GlfwWindow::SetFullscreen(bool enabled)
{
    if (m_IsFullscreen == enabled)
        return;

    if (enabled)
    {
        glfwGetWindowPos(m_WindowHandle, &m_WindowedX, &m_WindowedY);
        glfwGetWindowSize(m_WindowHandle, &m_WindowedWidth, &m_WindowedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_WindowHandle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        m_IsFullscreen = true;
    }
    else
    {
        glfwSetWindowMonitor(m_WindowHandle, nullptr, m_WindowedX, m_WindowedY, m_WindowedWidth, m_WindowedHeight, 0);
        m_IsFullscreen = false;
    }
}

void GlfwWindow::SetWindowIcon(const std::string& path)
{
    GLFWimage images[1];
    images[0].pixels = stbi_load(path.c_str(), &images[0].width, &images[0].height, 0, 4);
    if (images[0].pixels)
    {
        glfwSetWindowIcon(m_WindowHandle, 1, images);
        stbi_image_free(images[0].pixels);
    }
    else
    {
        CH_CORE_WARN("Failed to load window icon from {}", path);
    }
}

void GlfwWindow::SetVSync(bool enabled)
{
    m_VSync = enabled;
    glfwSwapInterval(m_VSync ? 1 : 0);
}

void GlfwWindow::SetAntialiasing(bool enabled)
{
    if(enabled)
    {
        glEnable(GL_MULTISAMPLE); 
    }
    else
    {
        glDisable(GL_MULTISAMPLE); 
    }
}

void GlfwWindow::SetTargetFramesPerSecond(int framesPerSecond)
{
    m_TargetFPS = framesPerSecond;
}

} // namespace CHEngine
