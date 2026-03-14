#ifndef CH_LINUX_WINDOW_H
#define CH_LINUX_WINDOW_H

#include "engine/core/window.h"
#include "raylib.h"

struct GLFWwindow;

namespace CHEngine
{

class LinuxWindow : public Window
{
public:
    LinuxWindow(const WindowProperties& properties);
    virtual ~LinuxWindow();

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
    virtual void SetWindowIcon(Image icon) override;

    virtual void* GetNativeWindow() const override { return m_WindowHandle; }

private:
    void Init(const WindowProperties& properties);
    void Shutdown();

private:
    GLFWwindow* m_WindowHandle = nullptr;
    int m_Width, m_Height;
    std::string m_Title;
    bool m_VSync = true;
};

} // namespace CHEngine

#endif // CH_LINUX_WINDOW_H
