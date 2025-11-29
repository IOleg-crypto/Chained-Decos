#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include "servers/rendering/Interfaces/IRenderManager.h"
#include <memory>
#include <string>
#include <vector>

// Forward declarations to reduce dependencies
class IRenderCommand;
class IGameRenderable;
class IMenuRenderable;
class ModelLoader;
class CollisionManager;
class CollisionDebugRenderer;
class ShaderManager;

class RenderManager : public IRenderManager
{
public:
    RenderManager();
    ~RenderManager() override;

    // Initialization
    bool Initialize() override;

    static void InitializeImGuiFont(const std::string &fontPath,
                                    float fontSize); // Delegates to ImGuiHelper

    // Main rendering methods
    void BeginFrame() const override;
    void EndFrame() override;

    // Command-based rendering (NEW APPROACH)
    void SubmitCommand(std::unique_ptr<IRenderCommand> command);
    void ExecuteCommands();
    void ClearCommands();

    // Legacy methods (DEPRECATED - will be removed)
    // These are kept temporarily for backward compatibility
    void RenderMenu(IMenuRenderable &renderable);

    // Debug rendering
    void ToggleDebugInfo();
    void ToggleCollisionDebug() override;
    void DrawCameraInfo(const Camera &camera, int cameraMode) const;
    void DrawModelManagerInfo(const ModelLoader &models) const;
    void DrawCollisionSystemInfo(const CollisionManager &collisionManager) const;
    void DrawControlsInfo() const;
    bool LoadWindShader();

    // Public rendering method for game world
    void RenderGame(IGameRenderable &renderable, const ModelLoader &models,
                    const CollisionManager &collisionManager,
                    bool showCollisionDebug = false) override;

    // Debug rendering (moved to public for GameApplication access)
    void ShowMetersPlayer(const IGameRenderable &renderable) const;
    void RenderDebugInfo(const IGameRenderable &renderable, const ModelLoader &models,
                         const CollisionManager &collision) const override;

    Font GetFont() const;

    bool IsCollisionDebugVisible() const override
    {
        return m_showCollisionDebug;
    }

private:
    void BeginMode3D(const Camera &camera);
    void EndMode3D();
    void DrawScene3D(const ModelLoader &models);
    void DrawPlayer(IGameRenderable &renderable, const ModelLoader &models);
    void RenderCollisionDebug(const CollisionManager &collisionManager,
                              IGameRenderable &renderable) const;

public:
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

#endif // RENDERMANAGER_H