#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

namespace CHEngine
{
class RenderManager
{
public:
    static void Init(int width = 1280, int height = 720, const char *title = "Chained Decos");
    static void Shutdown();
    static bool IsInitialized();

    static void Update(float deltaTime);

    // Main rendering
    static void BeginFrame();
    static void EndFrame();

    // 3D rendering
    static void BeginMode3D(Camera camera);
    static void EndMode3D();

    // Camera access
    static Camera &GetCamera();
    static void SetCamera(const Camera &camera);

    // Window
    static int GetScreenWidth();
    static int GetScreenHeight();
    static void SetBackgroundColor(Color color);
    static Color GetBackgroundColor();

    // Debug
    static void ToggleDebugInfo();
    static void SetDebugInfo(bool enabled);
    static bool IsDebugInfoVisible();
    static bool IsCollisionDebugVisible();
    static void SetCollisionDebugVisible(bool visible);

    // Font
    static Font GetFont();
    static void SetFont(Font font);

public:
    RenderManager();
    ~RenderManager();

    bool InternalInitialize(int width, int height, const char *title);
    void InternalShutdown();
    void InternalUpdate(float deltaTime);

    void InternalBeginFrame();
    void InternalEndFrame();

    void InternalBeginMode3D(Camera camera);
    void InternalEndMode3D();

    Camera &InternalGetCamera()
    {
        return m_camera;
    }
    void InternalSetCamera(const Camera &camera)
    {
        m_camera = camera;
    }

    int InternalGetScreenWidth() const;
    int InternalGetScreenHeight() const;
    void InternalSetBackgroundColor(Color color)
    {
        m_backgroundColor = color;
    }
    Color InternalGetBackgroundColor() const
    {
        return m_backgroundColor;
    }

    void InternalToggleDebugInfo()
    {
        m_showDebugInfo = !m_showDebugInfo;
    }
    void InternalSetDebugInfo(bool enabled)
    {
        m_showDebugInfo = enabled;
    }
    bool InternalIsDebugInfoVisible() const
    {
        return m_showDebugInfo;
    }
    bool InternalIsCollisionDebugVisible() const
    {
        return m_debugCollision;
    }
    void InternalSetCollisionDebugVisible(bool visible)
    {
        m_debugCollision = visible;
    }

    Font InternalGetFont() const
    {
        return m_font;
    }
    void InternalSetFont(Font font)
    {
        m_font = font;
    }

private:
    Camera m_camera = {0};
    Color m_backgroundColor = SKYBLUE;
    Font m_font{};

    bool m_showDebugInfo = false;
    bool m_debugCollision = false;
    bool m_initialized = false;

    int m_screenWidth = 1280;
    int m_screenHeight = 720;
};
} // namespace CHEngine

#endif // RENDERMANAGER_H
