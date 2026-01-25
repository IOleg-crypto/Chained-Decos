#include "window.h"
#include "engine/core/log.h"
#include <raylib.h>

// ImGui
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

// GLFW for getting native handle
#define GLFW_INCLUDE_NONE
#include "external/glfw/include/GLFW/glfw3.h"

namespace CHEngine
{
Window::Window(const WindowConfig &config)
    : m_Width(config.Width), m_Height(config.Height), m_Title(config.Title), m_VSync(config.VSync)
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
    // In Raylib, GetWindowHandle() returns HWND on Windows, but we need GLFWwindow*.
    // Since InitWindow makes the context current, we can get the GLFW handle this way:
    m_Window = glfwGetCurrentContext();

    CH_CORE_INFO("GLFW Window Handle obtained: {}", (void *)m_Window);

    if (!m_Window)
    {
        CH_CORE_ERROR("Failed to get GLFW window handle! Is GLFW initialized?");
        return;
    }

    // Setup ImGui with GLFW backend
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();

    // Enable docking and viewports
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Platform/Renderer backends
    // Now that we have the correct GLFWwindow*, we can install callbacks (true)
    if (!ImGui_ImplGlfw_InitForOpenGL(m_Window, true))
    {
        CH_CORE_ERROR("Failed to initialize ImGui GLFW backend");
        return;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 430"))
    {
        CH_CORE_ERROR("Failed to initialize ImGui OpenGL3 backend");
        return;
    }

    // Setup style
    ImGui::StyleColorsDark();

    // When viewports are enabled, tweak WindowRounding/WindowBg
    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    SetExitKey(KEY_NULL); // Prevent ESC from closing the app

    CH_CORE_INFO("Window Initialized with native ImGui backends: {} ({}x{})", m_Title, m_Width,
                 m_Height);
    CH_CORE_INFO("ImGui Viewports: {}",
                 (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) ? "Enabled" : "Disabled");
}

Window::~Window()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    CloseWindow();
    m_Window = nullptr;
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
    // Use a simpler clear if needed, but DARKGRAY is Raylib default
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
