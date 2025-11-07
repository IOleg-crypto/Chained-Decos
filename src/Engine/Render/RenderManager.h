#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include <memory>
#include <raylib.h>
#include <string>

// Include necessary headers
#include "IGameRenderable.h"
#include "IMenuRenderable.h"
#include "Model/Model.h"
#include "Collision/CollisionManager.h"
#include "Collision/CollisionDebugRenderer.h"
#include "Engine/Kernel/IKernelService.h"
#include "Shader/ShaderManager.h"

//
// RenderManager - Handles all rendering operations
// Separates rendering logic from the main Engine class
//
class RenderManager : public IKernelService
{
public:
    // Constructor
    explicit RenderManager();

    // Destructor
    ~RenderManager();

    // Initialization
    virtual bool Initialize() override;

    static void InitializeImGuiFont(const std::string &fontPath, float fontSize);

    // Main rendering methods
    void BeginFrame() const;
    void EndFrame();

    void RenderGame(IGameRenderable &renderable, const ModelLoader &models,
                     const CollisionManager &collisionManager, bool showCollisionDebug = false);
    void RenderMenu(IMenuRenderable &renderable);
    void RenderDebugInfo(IGameRenderable &renderable, const ModelLoader &models,
                          const CollisionManager &collisionManager);

    // 3D Scene rendering
    void BeginMode3D(const Camera &camera);
    void EndMode3D();
    void DrawScene3D(const ModelLoader &models);
    void DrawPlayer(IGameRenderable &renderable, const ModelLoader &models);

    // Debug rendering
    void RenderCollisionDebug(const CollisionManager &collisionManager, IGameRenderable &renderable) const;

    // Normal gameplay collision shape rendering
    void RenderCollisionShapes(const CollisionManager &collisionManager, IGameRenderable &renderable) const;

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

    void ShowMetersPlayer(const IGameRenderable &renderable) const;

    [[nodiscard]] Font GetFont() const;

    // IKernelService overrides
    virtual void Shutdown() override;
    virtual void Update(float deltaTime) override;
    virtual void Render() override;
    virtual const char *GetName() const override { return "RenderManager"; }
private:
    // Private helper methods for debug info
    void DrawDebugInfoWindow(IGameRenderable &renderable, const ModelLoader &models,
                              const CollisionManager &collisionManager);
    void DrawCameraInfo(const Camera &camera, int cameraMode);
    void DrawModelManagerInfo(const ModelLoader &models);

    void DrawCollisionSystemInfo(const CollisionManager &collisionManager);
    void DrawControlsInfo();
    
    // Shader loading helper
    bool LoadWindShader();
private:
    // Debug rendering
    std::unique_ptr<CollisionDebugRenderer> m_collisionDebugRenderer; // incomplete type is fine here
    // Debug state
    bool m_showDebugInfo = true;
    bool m_showCollisionDebug = false;
    bool m_forceCollisionDebugNextFrame = false;

    // Background color
    Color m_backgroundColor = BLUE;

    Font m_font{}; // Custom font for raylib
    
    // Shader management
    std::unique_ptr<ShaderManager> m_shaderManager;
    
    // Player wind effect shader uniforms
    int m_fallSpeedLoc = -1;
    int m_timeLoc = -1;
    int m_windDirectionLoc = -1;
    float m_shaderTime = 0.0f;
};

#endif // RENDER_MANAGER_H