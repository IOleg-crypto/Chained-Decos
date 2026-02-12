#ifndef CH_WINDOW_H
#define CH_WINDOW_H

#include <string>
#include <memory>

// Forward declare GLFWwindow
struct GLFWwindow;

namespace CHEngine
{
    class GraphicsContext;
struct WindowProperties
{
    std::string Title;
    int Width = 1280;
    int Height = 720;
    bool VSync = true;
    bool Resizable = true;
    bool Fullscreen = false;
    int TargetFramesPerSecond = 60;
    std::string IconPath = "";
    
    // UI / Docking
    bool EnableViewports = true;
    bool EnableDocking = true;
    std::string ImGuiConfigurationPath = "imgui.ini";

    WindowProperties(const std::string& title = "Chained Engine", int width = 1280, int height = 720)
        : Title(title), Width(width), Height(height) {}
};

class Window
{
public:
    Window(const WindowProperties &properties);
    ~Window();
    bool ShouldClose() const;
    void BeginFrame();
    void EndFrame();

    int GetWidth() const
    {
        return m_Width;
    }
    int GetHeight() const
    {
        return m_Height;
    }

    GLFWwindow *GetNativeWindow() const
    {
        return m_WindowHandle;
    }

    void SetTitle(const std::string &title);
    void ToggleFullscreen();

    void SetVSync(bool enabled);
    void SetTargetFramesPerSecond(int framesPerSecond);
    void SetWindowIcon(Image icon);

private:
    std::unique_ptr<GraphicsContext> m_Context;
    GLFWwindow *m_WindowHandle = nullptr;
    int m_Width, m_Height;
    std::string m_Title;
    bool m_VSync = true;
    std::string m_ImGuiConfigurationPath;
};
} // namespace CHEngine

#endif // CH_WINDOW_H
