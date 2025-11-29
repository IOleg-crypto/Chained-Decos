#ifndef IRENDERMANAGER_H
#define IRENDERMANAGER_H

#include <memory>
#include <raylib.h>
#include <string>
#include <vector>


class IRenderCommand;
class IGameRenderable;
class IMenuRenderable;
class ModelLoader;
class CollisionManager;

// Abstract interface for RenderManager
class IRenderManager
{
public:
    virtual ~IRenderManager() = default;

    // Initialization
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

    // Main rendering methods
    virtual void BeginFrame() const = 0;
    virtual void EndFrame() = 0;

    // Command-based rendering
    virtual void SubmitCommand(std::unique_ptr<IRenderCommand> command) = 0;
    virtual void ExecuteCommands() = 0;
    virtual void ClearCommands() = 0;

    // Debug rendering
    virtual void ToggleDebugInfo() = 0;
    virtual void ToggleCollisionDebug() = 0;
    virtual void ForceCollisionDebugNextFrame() = 0;
    virtual void SetDebugInfo(bool enabled) = 0;
    virtual void SetCollisionDebug(bool enabled) = 0;
    virtual bool IsDebugInfoVisible() const = 0;
    virtual bool IsCollisionDebugVisible() const = 0;

    // Game rendering
    virtual void RenderGame(IGameRenderable &renderable, const ModelLoader &models,
                            const CollisionManager &collisionManager,
                            bool showCollisionDebug = false) = 0;

    // Debug helpers
    virtual void ShowMetersPlayer(const IGameRenderable &renderable) const = 0;
    virtual void RenderDebugInfo(const IGameRenderable &renderable, const ModelLoader &models,
                                 const CollisionManager &collision) const = 0;

    virtual Font GetFont() const = 0;
};

#endif // IRENDERMANAGER_H
