#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include "core/macros.h"
#include "core/object/Object.h"
#include <raylib.h>

class RenderManager : public Object
{
    REGISTER_CLASS(RenderManager, Object)
    DISABLE_COPY_AND_MOVE(RenderManager)

public:
    // Constructor and Destructor
    RenderManager();
    ~RenderManager();

    // Lifecycle
    bool Initialize(int width, int height, const char *title);
    void Shutdown();
    void Update(float deltaTime);

    // Frame rendering
    void BeginFrame();
    void EndFrame();

    // 3D rendering
    void BeginMode3D(Camera camera);
    void EndMode3D();

    // Camera access
    Camera &GetCamera();
    const Camera &GetCamera() const;
    void SetCamera(const Camera &camera);

    // Window properties
    int GetScreenWidth() const;
    int GetScreenHeight() const;
    void SetBackgroundColor(Color color);
    Color GetBackgroundColor() const;

    // Debug
    void ToggleDebugInfo();
    void SetDebugInfo(bool enabled);
    bool IsDebugInfoVisible() const;
    bool IsCollisionDebugVisible() const;
    void SetCollisionDebugVisible(bool visible);

    // Font
    Font GetFont() const;
    void SetFont(Font font);

private:
    // State
    Camera m_camera;
    Color m_backgroundColor;
    Font m_font;

    bool m_showDebugInfo;
    bool m_debugCollision;
    bool m_initialized;

    int m_screenWidth;
    int m_screenHeight;
};

#endif // RENDERMANAGER_H