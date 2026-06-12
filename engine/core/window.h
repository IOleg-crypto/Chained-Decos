#ifndef CH_WINDOW_H
#define CH_WINDOW_H

#include "engine/core/events.h"
#include <GLFW/glfw3.h>
#include <memory>
#include <string>

namespace Chained
{
// Window creation parameters and UI/runtime integration flags.
struct WindowProperties
{
    std::string Title = "Chained Engine";
    int Width = 1280;
    int Height = 720;
    bool VSync = true;
    bool Resizable = true;
    bool Fullscreen = false;
    int TargetFramesPerSecond = 60; // 60hz is default for all monitors
    std::string IconPath;

    // UI / Docking
    bool EnableViewports = true;
    bool EnableDocking = true;
    std::string ImGuiConfigurationPath = "imgui.ini";
};

// Abstract native window interface used by the application and renderer.
class Window
{
public:
    virtual ~Window() = default;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    virtual bool ShouldClose() const = 0;

    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;

    virtual void SetTitle(const std::string& title) = 0;
    virtual void SetSize(int width, int height) = 0;
    virtual void SetSizeDirect(int width, int height) = 0;

    virtual void ToggleFullscreen() = 0;
    virtual void SetFullscreen(bool enabled) = 0;

    virtual void SetVSync(bool enabled) = 0;
    virtual void SetAntialiasing(bool enabled) = 0;
    virtual void SetTargetFramesPerSecond(int framesPerSecond) = 0;
    virtual int GetTargetFramesPerSecond() const = 0;
    virtual void SetWindowIcon(const std::string& path) = 0;

    virtual void* GetNativeWindow() const = 0;

    virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

    // Creates the platform-specific window implementation.
    static std::unique_ptr<Window> Create(const WindowProperties& properties = WindowProperties());
};
} // namespace Chained

#endif // CH_WINDOW_H
