#ifndef RENDER_MANAGER_H
#define RENDER_MANAGER_H

#include <memory>
#include <raylib.h>
#include <string>

#include <Collision/CollisionDebugRenderer.h>
#include <Collision/CollisionManager.h>
#include <Menu/Menu.h>
#include <Model/Model.h>
#include <Player/Player.h>

//
// RenderManager - Handles all rendering operations
// Separates rendering logic from the main Engine class
//
class RenderManager
{
public:
    // Constructor
    explicit RenderManager();

    // Destructor
    ~RenderManager();

    // Initialization
    void Initialize();
    void InitializeImGuiFont(const std::string &fontPath, float fontSize);

    // Main rendering methods
    void BeginFrame();
    void EndFrame();

    void RenderGame(const Player &player, const Models &models,
                    const CollisionManager &collisionManager, bool showCollisionDebug = false);
    void RenderMenu(Menu &menu);
    void RenderDebugInfo(const Player &player, const Models &models,
                         const CollisionManager &collisionManager);

    // 3D Scene rendering
    void BeginMode3D(const Camera &camera);
    void EndMode3D();
    void DrawScene3D(const Models &models);
    void DrawPlayer(const Player &player, const Models &models);

    // Debug rendering
    void RenderCollisionDebug(const CollisionManager &collisionManager, const Player &player) const;

    // Utility methods
    void SetBackgroundColor(Color color);
    void ToggleDebugInfo() { m_showDebugInfo = !m_showDebugInfo; }
    void ToggleCollisionDebug() { m_showCollisionDebug = !m_showCollisionDebug; }
    void ForceCollisionDebugNextFrame() { m_forceCollisionDebugNextFrame = true; }

    // Setters for debug modes
    void SetDebugInfo(bool enabled) { m_showDebugInfo = enabled; }
    void SetCollisionDebug(bool enabled) { m_showCollisionDebug = enabled; }

    // Getters
    bool IsDebugInfoVisible() const { return m_showDebugInfo; }
    bool IsCollisionDebugVisible() const { return m_showCollisionDebug; }

    void ShowMetersPlayer(const Player &player) const;

private:
    // Debug rendering
    std::unique_ptr<CollisionDebugRenderer> m_collisionDebugRenderer;

    // Private helper methods for debug info
    void DrawDebugInfoWindow(const Player &player, const Models &models,
                             const CollisionManager &collisionManager);
    void DrawCameraInfo(const Camera &camera, int cameraMode);
    void DrawModelManagerInfo(const Models &models);
    void DrawCollisionSystemInfo(const CollisionManager &collisionManager);
    void DrawControlsInfo();

    // Debug state
    bool m_showDebugInfo = false;
    bool m_showCollisionDebug = false;
    bool m_forceCollisionDebugNextFrame = false;

    // Background color
    Color m_backgroundColor = BLUE;

    Font m_font{}; // Custom font for raylib
};

#endif // RENDER_MANAGER_H