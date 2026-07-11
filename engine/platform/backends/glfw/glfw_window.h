#ifndef CH_GLFW_WINDOW_H
#define CH_GLFW_WINDOW_H

#include "engine/core/window.h"
#include <string>

struct GLFWwindow;

namespace Chained
{

// GLFW-backed window implementation that owns the native window, callbacks, and display settings.
class GlfwWindow : public Window
{
public:
    GlfwWindow(const WindowProperties& properties);
    virtual ~GlfwWindow();

    // Забороняємо копіювання вікна (Rule of 3/5), щоб уникнути подвійного видалення системного дескриптора
    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;

    virtual void BeginFrame() override;
    virtual void EndFrame() override;

    virtual bool ShouldClose() const override;

    // Повертають віртуальний розмір вікна в екранних координатах
    virtual int GetWidth() const override
    {
        return m_Width;
    }
    virtual int GetHeight() const override
    {
        return m_Height;
    }

    // Нові методи для отримання реального фізичного розміру буфера кадру в пікселях (для рендерингу)
    int GetFramebufferWidth() const
    {
        return m_FramebufferWidth;
    }
    int GetFramebufferHeight() const
    {
        return m_FramebufferHeight;
    }

    virtual void SetTitle(const std::string& title) override;
    virtual void SetSize(int width, int height) override;
    virtual void SetSizeDirect(int width, int height) override;

    virtual void ToggleFullscreen() override;
    virtual void SetFullscreen(bool enabled) override;

    virtual void SetVSync(bool enabled) override;
    virtual void SetAntialiasing(bool enabled) override;
    virtual void SetTargetFramesPerSecond(int framesPerSecond) override;
    virtual int GetTargetFramesPerSecond() const override
    {
        return m_TargetFPS;
    }
    virtual void SetWindowIcon(const std::string& path) override;
    virtual void SetCursorMode(CursorMode mode) override;

    virtual void SetEventCallback(const EventCallbackFn& callback) override
    {
        m_EventCallback = callback;
    }

    virtual void* GetNativeWindow() const override
    {
        return m_WindowHandle;
    }

private:
    void Init(const WindowProperties& properties);
    void Shutdown();

private:
    GLFWwindow* m_WindowHandle = nullptr;

    // Розмір вікна у віртуальних екранних координатах (використовується для подій миші та ОС)
    int m_Width = 0;
    int m_Height = 0;

    // Фізичний розмір буфера кадру в пікселях (використовується для glViewport)
    int m_FramebufferWidth = 0;
    int m_FramebufferHeight = 0;

    std::string m_Title;
    bool m_VSync = true;
    bool m_IsFullscreen = false;
    int m_WindowedX = 0, m_WindowedY = 0, m_WindowedWidth = 0, m_WindowedHeight = 0;
    int m_TargetFPS = 60;
    EventCallbackFn m_EventCallback;
};

} // namespace Chained

#endif // CH_GLFW_WINDOW_H