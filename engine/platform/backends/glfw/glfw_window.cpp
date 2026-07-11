#include "glfw_window.h"
#include "engine/common/engine_assert.h"
#include "engine/common/platform_detection.h"
#include "engine/core/events/window_events.h"
#include "engine/core/input.h"
#include "engine/core/log.h"

#include "glfw_input_mapper.h"
#include "imgui_impl_glfw.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <stb_image.h>

namespace Chained
{

static bool s_GLFWInitialized = false;

std::unique_ptr<Window> Window::Create(const WindowProperties& properties)
{
    // Наразі всі підтримувані платформи (Windows, Linux, MacOS) використовують GLFW
    return std::make_unique<GlfwWindow>(properties);
}

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

    if (!s_GLFWInitialized)
    {
        int success = glfwInit();
        CH_CORE_ASSERT(success, "Could not initialize GLFW!");
        glfwSetErrorCallback(GLFWErrorCallback);
        s_GLFWInitialized = true;
    }

    // Якщо розміри не вказані, беремо робочу область головного монітора
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

    m_Width = (uint32_t)initialWidth;
    m_Height = (uint32_t)initialHeight;
    CH_CORE_INFO("Initializing Glfw Window: {} ({}x{})", m_Title, m_Width, m_Height);

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef CH_PLATFORM_MACOS
    // macOS підтримує OpenGL максимум до версії 4.1 Core
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

    // Отримуємо реальний розмір буфера кадру у пікселях для glViewport (важливо для Retina/HiDPI)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_WindowHandle, &fbWidth, &fbHeight);
    m_FramebufferWidth = (uint32_t)fbWidth;
    m_FramebufferHeight = (uint32_t)fbHeight;

    // Зворотний виклик для зміни розміру вікна
    glfwSetFramebufferSizeCallback(m_WindowHandle, [](GLFWwindow* window, int width, int height) {
        auto& glWindow = *(GlfwWindow*)glfwGetWindowUserPointer(window);

        // Зберігаємо фізичний розмір у пікселях для рендерингу
        glWindow.m_FramebufferWidth = (uint32_t)width;
        glWindow.m_FramebufferHeight = (uint32_t)height;

        // Оновлюємо також віртуальний розмір вікна в екранних координатах
        int winWidth, winHeight;
        glfwGetWindowSize(window, &winWidth, &winHeight);
        glWindow.SetSizeDirect(winWidth, winHeight);

        WindowResizeEvent event(width, height);
        if (glWindow.m_EventCallback)
        {
            glWindow.m_EventCallback(event);
        }

        glViewport(0, 0, width, height);
    });

    // Зворотний виклик для закриття вікна
    glfwSetWindowCloseCallback(m_WindowHandle, [](GLFWwindow* window) {
        auto& glWindow = *(GlfwWindow*)glfwGetWindowUserPointer(window);
        WindowCloseEvent event;
        if (glWindow.m_EventCallback)
        {
            glWindow.m_EventCallback(event);
        }
    });

    // Миша: Скрол
    glfwSetScrollCallback(m_WindowHandle, [](GLFWwindow* window, double xOffset, double yOffset) {
        Core::Input::OnMouseScroll((float)xOffset, (float)yOffset);
        ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
    });

    // Клавіатура: Натискання клавіш
    glfwSetKeyCallback(m_WindowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        Core::Input::OnKey(GlfwInputMapper::MapKey(key), action != GLFW_RELEASE);
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    });

    // Миша: Кнопки
    glfwSetMouseButtonCallback(m_WindowHandle, [](GLFWwindow* window, int button, int action, int mods) {
        Core::Input::OnMouseButton(GlfwInputMapper::MapMouseButton(button), action != GLFW_RELEASE);
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    });

    // Миша: Рух курсора
    glfwSetCursorPosCallback(m_WindowHandle, [](GLFWwindow* window, double xpos, double ypos) {
        Core::Input::OnMouseMove((float)xpos, (float)ypos);
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
    });

    // Введення тексту для ImGui
    glfwSetCharCallback(m_WindowHandle,
                        [](GLFWwindow* window, unsigned int c) { ImGui_ImplGlfw_CharCallback(window, c); });

    // Втрата фокусу вікна — скидаємо стани, щоб клавіші не «залипали»
    glfwSetWindowFocusCallback(m_WindowHandle, [](GLFWwindow* window, int focused) {
        if (!focused)
        {
            Core::Input::ResetAll();
        }
    });

    // Ініціалізація GLAD
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
        // Відв'язуємо колбеки, щоб GLFW не викликав занулені вказівники на функції
        glfwSetScrollCallback(m_WindowHandle, nullptr);
        glfwSetKeyCallback(m_WindowHandle, nullptr);
        glfwSetMouseButtonCallback(m_WindowHandle, nullptr);
        glfwSetCursorPosCallback(m_WindowHandle, nullptr);
        glfwSetCharCallback(m_WindowHandle, nullptr);
        glfwSetWindowFocusCallback(m_WindowHandle, nullptr);
        glfwSetFramebufferSizeCallback(m_WindowHandle, nullptr);
        glfwSetWindowCloseCallback(m_WindowHandle, nullptr);

        glfwMakeContextCurrent(nullptr);
        glfwDestroyWindow(m_WindowHandle);
        m_WindowHandle = nullptr;
    }

    // glfwTerminate() НЕ викликаємо тут, оскільки воно має викликатися лише перед закриттям усього додатку.
    CH_CORE_INFO("Glfw Window Closed");
}

void GlfwWindow::BeginFrame()
{
    glfwPollEvents();
}

void GlfwWindow::EndFrame()
{
    glfwSwapBuffers(m_WindowHandle);
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
    m_Width = (uint32_t)width;
    m_Height = (uint32_t)height;
    // glfwSetWindowSize приймає екранні координати, а не пікселі буфера кадру
    glfwSetWindowSize(m_WindowHandle, (int)m_Width, (int)m_Height);
}

void GlfwWindow::SetSizeDirect(int width, int height)
{
    m_Width = (uint32_t)width;
    m_Height = (uint32_t)height;
}

void GlfwWindow::ToggleFullscreen()
{
    SetFullscreen(!m_IsFullscreen);
}

void GlfwWindow::SetFullscreen(bool enabled)
{
    if (m_IsFullscreen == enabled)
    {
        return;
    }

    if (enabled)
    {
        // Зберігаємо поточну позицію та розмір вікна перед переходом у повноекранний режим
        glfwGetWindowPos(m_WindowHandle, &m_WindowedX, &m_WindowedY);
        glfwGetWindowSize(m_WindowHandle, &m_WindowedWidth, &m_WindowedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(m_WindowHandle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        m_IsFullscreen = true;
    }
    else
    {
        // Повертаємо віконний режим. Замість '0' передаємо 'GLFW_DONT_CARE' для частоти оновлення вікна
        glfwSetWindowMonitor(m_WindowHandle, nullptr, m_WindowedX, m_WindowedY, m_WindowedWidth, m_WindowedHeight,
                             GLFW_DONT_CARE);
        m_IsFullscreen = false;
    }
}

void GlfwWindow::SetWindowIcon(const std::string& path)
{
    GLFWimage image{};

    // Іконки для GLFW завантажуються без вертикального перевертання.
    // Зберігаємо поточний стан прапорця stb_image, щоб не зламати рендеринг текстур в інших частинах рушія.
    // За замовчуванням у stb_image цей прапорець false, але якщо у вас у рушії true — цей підхід безпечний.
    image.pixels = stbi_load(path.c_str(), &image.width, &image.height, nullptr, 4);

    if (image.pixels)
    {
        glfwSetWindowIcon(m_WindowHandle, 1, &image);
        stbi_image_free(image.pixels);
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
    if (enabled)
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

void GlfwWindow::SetCursorMode(CursorMode mode)
{
    switch (mode)
    {
    case CursorMode::Normal:
        glfwSetInputMode(m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        break;
    case CursorMode::Hidden:
        glfwSetInputMode(m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        break;
    case CursorMode::Locked:
        glfwSetInputMode(m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        break;
    }
}

} // namespace Chained