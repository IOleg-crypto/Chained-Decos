#include "sdl_window.h"
#include "sdl_input_mapper.h"
#include "engine/core/log.h"
#include "engine/core/input.h"
#include "engine/core/events/window_events.h"
#include "engine/core/events/input_events.h"

#include <SDL3/SDL.h>
#include <glad/gl.h>
#include <stb_image.h>
#include <imgui_impl_sdl3.h>

namespace Chained
{

static SDL_GLContext s_GLContext = nullptr;
static bool s_SDLInitialized = false;

std::unique_ptr<Window> Window::Create(const WindowProperties& properties)
{
    return std::make_unique<SdlWindow>(properties);
}

SdlWindow::SdlWindow(const WindowProperties& properties)
{
    Init(properties);
}

SdlWindow::~SdlWindow()
{
    Shutdown();
}

void SdlWindow::Init(const WindowProperties& properties)
{
    m_Title = properties.Title;
    m_VSync = properties.VSync;
    m_TargetFPS = properties.TargetFramesPerSecond;

    int initialWidth = properties.Width;
    int initialHeight = properties.Height;

    if (!s_SDLInitialized)
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            CH_CORE_ERROR("SDL3: Failed to initialize SDL: {}", SDL_GetError());
            return;
        }
        s_SDLInitialized = true;
    }

    // Fallback to native monitor resolution when size is 0x0
    if (initialWidth <= 0 || initialHeight <= 0)
    {
        SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(displayID);
        if (mode)
        {
            initialWidth = (mode->w > 0) ? mode->w : 1280;
            initialHeight = (mode->h > 0) ? mode->h : 720;
        }
        else
        {
            initialWidth = 1280;
            initialHeight = 720;
        }
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags flags = static_cast<SDL_WindowFlags>(
        SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );
    if (properties.Resizable)
    {
        flags = static_cast<SDL_WindowFlags>(flags | SDL_WINDOW_RESIZABLE);
    }
    if (properties.Fullscreen)
    {
        flags = static_cast<SDL_WindowFlags>(flags | SDL_WINDOW_FULLSCREEN);
    }

    m_WindowHandle = SDL_CreateWindow(
        m_Title.c_str(),
        initialWidth, initialHeight,
        flags
    );

    if (!m_WindowHandle)
    {
        CH_CORE_ERROR("SDL3: Failed to create window: {}", SDL_GetError());
        return;
    }

    // Center the window on the display
    {
        SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
        SDL_Rect usableBounds;
        if (SDL_GetDisplayUsableBounds(displayID, &usableBounds))
        {
            int centerX = (usableBounds.w - initialWidth) / 2 + usableBounds.x;
            int centerY = (usableBounds.h - initialHeight) / 2 + usableBounds.y;
            SDL_SetWindowPosition(m_WindowHandle, centerX, centerY);
        }
    }

    s_GLContext = SDL_GL_CreateContext(m_WindowHandle);
    if (!s_GLContext)
    {
        CH_CORE_ERROR("SDL3: Failed to create OpenGL context: {}", SDL_GetError());
        return;
    }

    SDL_GL_MakeCurrent(m_WindowHandle, s_GLContext);

    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress))
    {
        CH_CORE_ERROR("SDL3: Failed to load OpenGL via GLAD");
        return;
    }

    CH_CORE_INFO("SDL3 OpenGL {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    CH_CORE_INFO("  Vendor: {}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    CH_CORE_INFO("  Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    // Query actual framebuffer size (accounts for DPI scaling)
    int fbWidth = 0, fbHeight = 0;
    SDL_GetWindowSizeInPixels(m_WindowHandle, &fbWidth, &fbHeight);
    m_Width = fbWidth > 0 ? fbWidth : initialWidth;
    m_Height = fbHeight > 0 ? fbHeight : initialHeight;
    glViewport(0, 0, m_Width, m_Height);

    CH_CORE_INFO("SDL3 Window created: {} ({}x{})", m_Title, m_Width, m_Height);

    SetVSync(m_VSync);

    if (properties.Fullscreen)
    {
        m_IsFullscreen = true;
        m_WindowedWidth = initialWidth;
        m_WindowedHeight = initialHeight;
    }
}

void SdlWindow::Shutdown()
{
    if (s_GLContext)
    {
        SDL_GL_DestroyContext(s_GLContext);
        s_GLContext = nullptr;
    }
    if (m_WindowHandle)
    {
        SDL_DestroyWindow(m_WindowHandle);
        m_WindowHandle = nullptr;
    }
    // Don't call SDL_Quit() here — other parts of the app may still use SDL
}

void SdlWindow::BeginFrame()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
        {
            m_ShouldClose = true;
            break;
        }
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        {
            m_ShouldClose = true;
            break;
        }
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        {
            Core::Input::ResetAll();
            break;
        }
        case SDL_EVENT_WINDOW_RESIZED:
        {
            int w = event.window.data1;
            int h = event.window.data2;
            if (w > 0 && h > 0)
            {
                m_Width = w;
                m_Height = h;

                // On high-DPI, window size != framebuffer size
                int fbWidth = 0, fbHeight = 0;
                SDL_GetWindowSizeInPixels(m_WindowHandle, &fbWidth, &fbHeight);
                if (fbWidth > 0 && fbHeight > 0)
                {
                    m_Width = fbWidth;
                    m_Height = fbHeight;
                }

                glViewport(0, 0, m_Width, m_Height);
                WindowResizeEvent e(m_Width, m_Height);
                if (m_EventCallback) m_EventCallback(e);
            }
            break;
        }
        case SDL_EVENT_KEY_DOWN:
        {
            KeyCode key = SdlInputMapper::MapKey(event.key.scancode);
            Core::Input::OnKey(key, true);
            KeyPressedEvent e(key, 0);
            if (m_EventCallback) m_EventCallback(e);
            break;
        }
        case SDL_EVENT_KEY_UP:
        {
            KeyCode key = SdlInputMapper::MapKey(event.key.scancode);
            Core::Input::OnKey(key, false);
            KeyReleasedEvent e(key);
            if (m_EventCallback) m_EventCallback(e);
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            MouseCode btn = SdlInputMapper::MapMouseButton(event.button.button);
            Core::Input::OnMouseButton(btn, true);
            MouseButtonPressedEvent e(btn);
            if (m_EventCallback) m_EventCallback(e);
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            MouseCode btn = SdlInputMapper::MapMouseButton(event.button.button);
            Core::Input::OnMouseButton(btn, false);
            MouseButtonReleasedEvent e(btn);
            if (m_EventCallback) m_EventCallback(e);
            break;
        }
        case SDL_EVENT_MOUSE_MOTION:
        {
            Core::Input::OnMouseMove((float)event.motion.x, (float)event.motion.y);
            MouseMovedEvent e((float)event.motion.x, (float)event.motion.y);
            if (m_EventCallback) m_EventCallback(e);
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            Core::Input::OnMouseScroll((float)event.wheel.x, (float)event.wheel.y);
            MouseScrolledEvent e((float)event.wheel.x, (float)event.wheel.y);
            if (m_EventCallback) m_EventCallback(e);
            break;
        }
        
        }
    }
}

void SdlWindow::EndFrame()
{
    SDL_GL_SwapWindow(m_WindowHandle);
}

bool SdlWindow::ShouldClose() const
{
    return m_ShouldClose;
}

void SdlWindow::SetTitle(const std::string& title)
{
    m_Title = title;
    SDL_SetWindowTitle(m_WindowHandle, title.c_str());
}

void SdlWindow::SetSize(int width, int height)
{
    m_Width = width;
    m_Height = height;
    SDL_SetWindowSize(m_WindowHandle, width, height);
}

void SdlWindow::SetSizeDirect(int width, int height)
{
    m_Width = width;
    m_Height = height;
}

void SdlWindow::ToggleFullscreen()
{
    SetFullscreen(!m_IsFullscreen);
}

void SdlWindow::SetFullscreen(bool enabled)
{
    if (enabled == m_IsFullscreen) return;

    if (enabled)
    {
        int x, y;
        SDL_GetWindowPosition(m_WindowHandle, &x, &y);
        m_WindowedX = x;
        m_WindowedY = y;
        m_WindowedWidth = m_Width;
        m_WindowedHeight = m_Height;
        SDL_SetWindowFullscreen(m_WindowHandle, SDL_WINDOW_FULLSCREEN);
    }
    else
    {
        SDL_SetWindowFullscreen(m_WindowHandle, 0);
        SDL_SetWindowPosition(m_WindowHandle, m_WindowedX, m_WindowedY);
        SDL_SetWindowSize(m_WindowHandle, m_WindowedWidth, m_WindowedHeight);
    }

    m_IsFullscreen = enabled;
}

void SdlWindow::SetVSync(bool enabled)
{
    m_VSync = enabled;
    SDL_GL_SetSwapInterval(enabled ? 1 : 0);
}

void SdlWindow::SetAntialiasing(bool enabled)
{
    if (enabled)
        glEnable(GL_MULTISAMPLE);
    else
        glDisable(GL_MULTISAMPLE);
}

void SdlWindow::SetTargetFramesPerSecond(int framesPerSecond)
{
    m_TargetFPS = framesPerSecond;
}

void SdlWindow::SetWindowIcon(const std::string& path)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (pixels)
    {
        SDL_Surface* surface = SDL_CreateSurfaceFrom(
            width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * 4);
        if (surface)
        {
            SDL_SetWindowIcon(m_WindowHandle, surface);
            SDL_DestroySurface(surface);
        }
        stbi_image_free(pixels);
    }
    else
    {
        CH_CORE_WARN("Failed to load window icon from {}", path);
    }
}

void SdlWindow::SetCursorMode(CursorMode mode)
{
    switch (mode)
    {
    case CursorMode::Normal:
        SDL_ShowCursor();
        SDL_SetWindowRelativeMouseMode(m_WindowHandle, false);
        break;
    case CursorMode::Hidden:
        SDL_HideCursor();
        SDL_SetWindowRelativeMouseMode(m_WindowHandle, false);
        break;
    case CursorMode::Locked:
        SDL_SetWindowRelativeMouseMode(m_WindowHandle, true);
        break;
    }
}

} // namespace Chained
