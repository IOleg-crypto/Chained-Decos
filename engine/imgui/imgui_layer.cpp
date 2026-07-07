#include "imgui_layer.h"
#include "engine/core/profiler.h"
#include "engine/core/window.h"


#include "imgui.h"
#include "ImGuizmo.h"
#include "engine/app/application.h"
#include "backends/imgui_impl_opengl3.h"

#ifdef CH_PLATFORM_BACKEND_GLFW
#include "backends/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>
#elif defined(CH_PLATFORM_BACKEND_SDL3)
#include "backends/imgui_impl_sdl3.h"
#include <SDL3/SDL.h>
#endif

namespace Chained
{
void ImGuiLayer::SetContext(ImGuiContext* context)
{
    ImGui::SetCurrentContext(context);
}

ImGuiLayer::ImGuiLayer()
{
}

ImGuiLayer::~ImGuiLayer()
{
}

void ImGuiLayer::OnAttach()
{
    CH_PROFILE_FUNCTION();

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();


    // Docking and viewports are always enabled
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup style
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

#ifdef CH_PLATFORM_BACKEND_GLFW
    GLFWwindow* window = (GLFWwindow*)Application::Get().GetWindow().GetNativeWindow();
    if (!window)
    {
        CH_CORE_ERROR("ImGuiLayer: Failed to get native window handle!");
        return;
    }
    ImGui_ImplGlfw_InitForOpenGL(window, false);
#elif defined(CH_PLATFORM_BACKEND_SDL3)
    SDL_Window* window = (SDL_Window*)Application::Get().GetWindow().GetNativeWindow();
    if (!window)
    {
        CH_CORE_ERROR("ImGuiLayer: Failed to get native window handle!");
        return;
    }
    ImGui_ImplSDL3_InitForOpenGL(window, SDL_GL_GetCurrentContext());
#endif
    ImGui_ImplOpenGL3_Init("#version 430");
}

void ImGuiLayer::OnDetach()
{
    CH_PROFILE_FUNCTION();

    ImGui::DestroyPlatformWindows();
    ImGui_ImplOpenGL3_Shutdown();
#ifdef CH_PLATFORM_BACKEND_GLFW
    ImGui_ImplGlfw_Shutdown();
#elif defined(CH_PLATFORM_BACKEND_SDL3)
    ImGui_ImplSDL3_Shutdown();
#endif
    ImGui::DestroyContext();
}

void ImGuiLayer::Begin()
{
    CH_PROFILE_FUNCTION();

    ImGui_ImplOpenGL3_NewFrame();
#ifdef CH_PLATFORM_BACKEND_GLFW
    ImGui_ImplGlfw_NewFrame();
#elif defined(CH_PLATFORM_BACKEND_SDL3)
    ImGui_ImplSDL3_NewFrame();
#endif
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void ImGuiLayer::End()
{
    CH_PROFILE_FUNCTION();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)Application::Get().GetWindow().GetWidth(), (float)Application::Get().GetWindow().GetHeight());

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
#ifdef CH_PLATFORM_BACKEND_GLFW
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
#elif defined(CH_PLATFORM_BACKEND_SDL3)
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
#endif
    }
}

bool ImGuiLayer::RefreshFontAtlasTexture()
{
    if (!ImGui::GetCurrentContext())
    {
        CH_CORE_WARN("ImGuiLayer: Cannot refresh font atlas without an active ImGui context.");
        return false;
    }

    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    if (!ImGui_ImplOpenGL3_CreateDeviceObjects())
    {
        CH_CORE_ERROR("ImGuiLayer: Failed to recreate OpenGL device objects for font atlas refresh.");
        return false;
    }

    CH_CORE_INFO("ImGuiLayer: Refreshed font atlas texture.");
    return true;
}

void ImGuiLayer::OnEvent(Event& e)
{
    if (m_BlockEvents)
    {
        ImGuiIO& io = ImGui::GetIO();
        e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
        e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
    }
}

void* ImGuiLayer::AddFontFromFile(const std::string& path, float size, const void* config, const void* ranges)
{
    if (!ImGui::GetCurrentContext())
    {
        CH_CORE_WARN("ImGuiLayer: Cannot add font without an active ImGui context.");
        return nullptr;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), size, (const ImFontConfig*)config, (const ImWchar*)ranges);
    if (!font)
    {
        CH_CORE_ERROR("ImGuiLayer: Failed to load font from '{}'", path);
    }
    return font;
}
} // namespace Chained
