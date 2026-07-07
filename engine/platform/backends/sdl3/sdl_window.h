#ifndef CH_SDL_WINDOW_H
#define CH_SDL_WINDOW_H

#include "engine/core/window.h"
#include <string>

struct SDL_Window;

namespace Chained
{
class SdlWindow : public Window
{
public:
    SdlWindow(const WindowProperties& properties);
    virtual ~SdlWindow();

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
    virtual void SetCursorMode(CursorMode mode) override;

    virtual void SetEventCallback(const EventCallbackFn& callback) override { m_EventCallback = callback; }

    virtual void* GetNativeWindow() const override { return m_WindowHandle; }

private:
    void Init(const WindowProperties& properties);
    void Shutdown();

private:
    SDL_Window* m_WindowHandle = nullptr;
    int m_Width = 0;
    int m_Height = 0;
    std::string m_Title;
    bool m_VSync = true;
    bool m_IsFullscreen = false;
    bool m_ShouldClose = false;
    int m_WindowedX = 0, m_WindowedY = 0, m_WindowedWidth = 0, m_WindowedHeight = 0;
    int m_TargetFPS = 60;
    EventCallbackFn m_EventCallback;
};

} // namespace Chained

#endif // CH_SDL_WINDOW_H
