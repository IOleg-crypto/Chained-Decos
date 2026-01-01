#ifndef ILEVEL_MANAGER_H
#define ILEVEL_MANAGER_H

#include "core/interfaces/IEngineModule.h"
#include <raylib.h>
#include <string>
#include <vector>

namespace CHEngine
{
class Scene;
class GameScene;

// Engine depends on this interface, not concrete LevelManager class.
class ILevelManager : public IEngineModule
{
public:
    virtual ~ILevelManager() = default;

    // ECS Scene Integration
    virtual void SetActiveScene(std::shared_ptr<CHEngine::Scene> scene) = 0;

    // Map lifecycle
    virtual bool LoadScene(const std::string &path) = 0;
    virtual bool LoadSceneByIndex(int index) = 0;
    virtual bool LoadSceneByName(const std::string &name) = 0;
    virtual bool LoadUIScene(const std::string &path) = 0;
    virtual void UnloadMap() = 0;
    virtual void UnloadUIScene() = 0;
    virtual bool IsMapLoaded() const = 0;
    virtual const std::string &GetCurrentMapPath() const = 0;

    // Map info
    virtual std::string GetCurrentMapName() const = 0;
    virtual Vector3 GetSpawnPosition() const = 0;

    // Collision
    virtual void InitCollisions() = 0;
    virtual bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels) = 0;

    // Rendering specialization if needed
    virtual void RenderEditorMap() = 0;

    // ECS Synchronization
    virtual void RefreshMapEntities() = 0;
    virtual void RefreshUIEntities() = 0;
    virtual void SyncEntitiesToMap() = 0;
    virtual GameScene &GetGameScene() = 0;
};
} // namespace CHEngine

#endif // ILEVEL_MANAGER_H
