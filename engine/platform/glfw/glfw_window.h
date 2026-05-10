#ifndef CH_GLFW_WINDOW_H
#define CH_GLFW_WINDOW_H

#include "engine/core/window.h"
#include <string>

struct GLFWwindow;

// GLFW-backed window implementation that owns the native window, callbacks, and display settings.
namespace CHEngine
{
class GlfwWindow : public Window
{
public:
    GlfwWindow(const WindowProperties& properties);
    virtual ~GlfwWindow();

    virtual void BeginFrame() override;
    virtual void EndFrame() override;

    virtual bool ShouldClose() const override;

    virtual int GetWidth() const override { return m_Width; }
    virtual int GetHeight() const override { return m_Height; }

    virtual void SetTitle(const std::string& title) override;
    virtual void SetSize(int width, int height) override;
    virtual void SetSizeDirect(int width, int height) override;

    virtual void ToggleFullscreen() override;
    virtual void SetFullscreen(bool enabled) override;

    virtual void SetVSync(bool enabled) override;
    virtual void SetAntialiasing(bool enabled) override;
    virtual void SetTargetFramesPerSecond(int framesPerSecond) override;
    virtual int GetTargetFramesPerSecond() const override { return m_TargetFPS; }
    virtual void SetWindowIcon(const std::string& path) override;

    virtual void SetEventCallback(const EventCallbackFn& callback) override { m_EventCallback = callback; }

    virtual void* GetNativeWindow() const override { return m_WindowHandle; }

private:
    void Init(const WindowProperties& properties);
    void Shutdown();

private:
    GLFWwindow* m_WindowHandle = nullptr;
    int m_Width, m_Height;
    std::string m_Title;
    bool m_VSync = true;
    bool m_IsFullscreen = false;
    int m_WindowedX = 0, m_WindowedY = 0, m_WindowedWidth = 0, m_WindowedHeight = 0;
    int m_TargetFPS = 60;
    EventCallbackFn m_EventCallback;
};

} // namespace CHEngine

#endif // CH_GLFW_WINDOW_H
