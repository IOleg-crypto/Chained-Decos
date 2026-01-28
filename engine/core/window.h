#ifndef CH_WINDOW_H
#define CH_WINDOW_H

#include "string"

// Forward declare GLFWwindow
struct GLFWwindow;

namespace CHEngine
{
struct WindowConfig
{
    std::string Title;
    int Width = 1280;
    int Height = 720;
    bool VSync = true;
    bool Resizable = true;
    bool Fullscreen = false;
    int TargetFPS = 60;
};

class Window
{
public:
    Window(const WindowConfig &config);
    ~Window();

    void PollEvents();
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
        return m_Window;
    }

    void SetTitle(const std::string &title);
    void ToggleFullscreen();

    void SetVSync(bool enabled);
    void SetTargetFPS(int fps);
    void SetWindowIcon(Image icon);

private:
    GLFWwindow *m_Window = nullptr;
    int m_Width, m_Height;
    std::string m_Title;
    bool m_VSync = true;
};
} // namespace CHEngine

#endif // CH_WINDOW_H
