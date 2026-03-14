#ifndef CH_WINDOW_H
#define CH_WINDOW_H

#include <memory>
#include <string>

// Forward declare GLFWwindow
struct GLFWwindow;

namespace CHEngine
{
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
        : Title(title),
          Width(width),
          Height(height)
    {
    }
};

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
    // Note: Image is from Raylib, might need abstraction later if moving away from Raylib
    // But for now we focus on the Window Interface.
    virtual void SetWindowIcon(Image icon) = 0;

    virtual void* GetNativeWindow() const = 0;

    static std::unique_ptr<Window> Create(const WindowProperties& properties = WindowProperties());
};
} // namespace CHEngine

#endif // CH_WINDOW_H
