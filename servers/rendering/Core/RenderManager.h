#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include <memory>
#include <raylib.h>
#include <string>

// Forward declarations to reduce dependencies
class IRenderCommand;
class IGameRenderable;
class IMenuRenderable;
class ModelLoader;
class CollisionManager;
class CollisionDebugRenderer;
class ShaderManager;

// Include only necessary interfaces
#include "core/object/kernel/Interfaces/IKernelService.h"
#include <memory>
#include <vector>

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

    static void InitializeImGuiFont(const std::string &fontPath,
                                    float fontSize); // Delegates to ImGuiHelper

    // Main rendering methods
    void BeginFrame() const;
    void EndFrame();

    // Command-based rendering (NEW APPROACH)
    void SubmitCommand(std::unique_ptr<IRenderCommand> command);
    void ExecuteCommands();
    void ClearCommands();

    // Legacy methods (DEPRECATED - will be removed)
    // These are kept temporarily for backward compatibility
    void RenderMenu(IMenuRenderable &renderable);

    // Debug rendering
    void ToggleDebugInfo();
    void ToggleCollisionDebug();
    void ForceCollisionDebugNextFrame();
    void SetDebugInfo(bool enabled);
    void SetCollisionDebug(bool enabled);
    [[nodiscard]] bool IsDebugInfoVisible() const;
    [[nodiscard]] bool IsCollisionDebugVisible() const;

    // IKernelService overrides
    virtual void Shutdown() override;
    virtual void Update(float deltaTime) override;
    virtual void Render() override;
    virtual const char *GetName() const override
    {
        return "RenderManager";
    }

private:
    // Private helper methods (still used internally)
    void RenderGame(IGameRenderable &renderable, const ModelLoader &models,
                    const CollisionManager &collisionManager, bool showCollisionDebug = false);
    void RenderDebugInfo(IGameRenderable &renderable, const ModelLoader &models,
                         const CollisionManager &collisionManager);
    void BeginMode3D(const Camera &camera);
    void EndMode3D();
    void DrawScene3D(const ModelLoader &models);
    void DrawPlayer(IGameRenderable &renderable, const ModelLoader &models);
    void RenderCollisionDebug(const CollisionManager &collisionManager,
                              IGameRenderable &renderable) const;
    void RenderCollisionShapes(const CollisionManager &collisionManager,
                               IGameRenderable &renderable) const;
    void SetBackgroundColor(Color color);
    void DrawDebugInfoWindow(IGameRenderable &renderable, const ModelLoader &models,
                             const CollisionManager &collisionManager);
    void DrawCameraInfo(const Camera &camera, int cameraMode);
    void DrawModelManagerInfo(const ModelLoader &models);
    void DrawCollisionSystemInfo(const CollisionManager &collisionManager);
    void DrawControlsInfo();
    void ShowMetersPlayer(const IGameRenderable &renderable) const;
    Font GetFont() const;
    bool LoadWindShader();

    // Command queue for rendering
    std::vector<std::unique_ptr<IRenderCommand>> m_commandQueue;

    // Debug rendering
    std::unique_ptr<CollisionDebugRenderer>
        m_collisionDebugRenderer; // incomplete type is fine here
    // Debug state
    bool m_showDebugInfo = false;
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