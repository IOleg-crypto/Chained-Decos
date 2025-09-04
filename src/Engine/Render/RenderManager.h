#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include <memory>
#include <raylib.h>
#include <string>

#include "Engine/Collision/CollisionDebugRenderer.h"
#include "Engine/Collision/CollisionManager.h"
#include "Game/Menu/Menu.h"
#include "Engine/Model/Model.h"

//
// RenderManager - Handles all rendering operations
// Separates rendering logic from the main Engine class
//

class Player;
class Menu;

class RenderManager
{
public:
    // Constructor
    explicit RenderManager();

    // Destructor
    ~RenderManager();

    // Initialization
    void Initialize();

    static void InitializeImGuiFont(const std::string &fontPath, float fontSize);

    // Main rendering methods
    void BeginFrame() const;
    void EndFrame();
    void Render();

    void RenderGame(const Player &player, const ModelLoader &models,
                    const CollisionManager &collisionManager, bool showCollisionDebug = false);
    void RenderMenu(Menu &menu);
    void RenderDebugInfo(const Player &player, const ModelLoader &models,
                         const CollisionManager &collisionManager);

    // 3D Scene rendering
    void BeginMode3D(const Camera &camera);
    void EndMode3D();
    void DrawScene3D(const ModelLoader &models);
    void DrawPlayer(const Player &player, const ModelLoader &models);

    // Debug rendering
    void RenderCollisionDebug(const CollisionManager &collisionManager, const Player &player) const;

    // Utility methods
    void SetBackgroundColor(Color color);
    void ToggleDebugInfo();

    void ToggleCollisionDebug();

    void ForceCollisionDebugNextFrame();

    // Setters for debug modes
    void SetDebugInfo(bool enabled);

    void SetCollisionDebug(bool enabled);

    // Getters
    [[nodiscard]] bool IsDebugInfoVisible() const;

    [[nodiscard]] bool IsCollisionDebugVisible() const;

    void ShowMetersPlayer(const Player &player) const;

    [[nodiscard]] Font GetFont() const;
private:
    // Private helper methods for debug info
    void DrawDebugInfoWindow(const Player &player, const ModelLoader &models,
                             const CollisionManager &collisionManager);
    void DrawCameraInfo(const Camera &camera, int cameraMode);
    void DrawModelManagerInfo(const ModelLoader &models);


    void DrawCollisionSystemInfo(const CollisionManager &collisionManager);
    void DrawControlsInfo();
private:
    // Debug rendering
    std::unique_ptr<CollisionDebugRenderer> m_collisionDebugRenderer;
    // Debug state
    bool m_showDebugInfo = true;
    bool m_showCollisionDebug = false;
    bool m_forceCollisionDebugNextFrame = false;

    // Background color
    Color m_backgroundColor = BLUE;

    Font m_font{}; // Custom font for raylib
};

#endif // RENDER_MANAGER_H