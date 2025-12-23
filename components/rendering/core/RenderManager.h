#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

class RenderManager
{
public:
    // Initialization
    RenderManager();
    ~RenderManager();

    // Initialization
    bool Initialize(int width, int height, const char *title);
    void Shutdown();
    void Update(float deltaTime);

    // Main rendering
    void BeginFrame();
    void EndFrame();

    // 3D rendering
    void BeginMode3D(Camera camera);
    void EndMode3D();

    // Camera access
    Camera &GetCamera()
    {
        return m_camera;
    }
    const Camera &GetCamera() const
    {
        return m_camera;
    }
    void SetCamera(const Camera &camera)
    {
        m_camera = camera;
    }

    // Window
    int GetScreenWidth() const;
    int GetScreenHeight() const;
    void SetBackgroundColor(Color color)
    {
        m_backgroundColor = color;
    }
    Color GetBackgroundColor() const
    {
        return m_backgroundColor;
    }

    // Debug
    void ToggleDebugInfo()
    {
        m_showDebugInfo = !m_showDebugInfo;
    }
    void SetDebugInfo(bool enabled)
    {
        m_showDebugInfo = enabled;
    }
    bool IsDebugInfoVisible() const
    {
        return m_showDebugInfo;
    }
    bool IsCollisionDebugVisible() const
    {
        return m_debugCollision;
    }
    void SetCollisionDebugVisible(bool visible)
    {
        m_debugCollision = visible;
    }

    // Font
    Font GetFont() const
    {
        return m_font;
    }
    void SetFont(Font font)
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

#endif // RENDERMANAGER_H
