#pragma once
#include "events/event.h"
#include <functional>
#include <string>

namespace CHEngine
{
using EventCallbackFn = std::function<void(Event &)>;

struct WindowProps
{
    std::string Title;
    int Width;
    int Height;
    bool Fullscreen;
    bool VSync;

    WindowProps(const std::string &title = "Chained Decos", int width = 1280, int height = 720,
                bool fullscreen = false, bool vsync = true)
        : Title(title), Width(width), Height(height), Fullscreen(fullscreen), VSync(vsync)
    {
    }
};

class Window
{
public:
    Window(const WindowProps &props);
    ~Window();

    void OnUpdate();

    int GetWidth() const;
    int GetHeight() const;
    void SetEventCallback(const EventCallbackFn &callback)
    {
        m_Data.EventCallback = callback;
    }

    // Attributes
    void SetVSync(bool enabled);
    bool IsVSync() const;

    void SetTitle(const std::string &title);
    void SetIcon(const std::string &path);

    bool ShouldClose() const;

private:
    void Init(const WindowProps &props);
    void Shutdown();

    struct WindowData
    {
        std::string Title;
        int Width, Height;
        bool VSync;
        bool Fullscreen;

        EventCallbackFn EventCallback;
    };

    WindowData m_Data;
};

} // namespace CHEngine
