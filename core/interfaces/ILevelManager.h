#ifndef ILEVEL_MANAGER_H
#define ILEVEL_MANAGER_H

#include "core/object/module/Interfaces/IEngineModule.h"
#include <raylib.h>
#include <string>
#include <vector>

// Minimal LevelManager interface
// Essential API only - 7 methods (down from 20+)
// Engine depends on this interface, not concrete LevelManager class.
class ILevelManager : public IEngineModule
{
public:
    virtual ~ILevelManager() = default;

    // Map lifecycle
    virtual bool LoadMap(const std::string &path) = 0;
    virtual void UnloadMap() = 0;
    virtual bool IsMapLoaded() const = 0;
    virtual const std::string &GetCurrentMapPath() const = 0;

    // Map info
    virtual std::string GetCurrentMapName() const = 0;
    virtual Vector3 GetSpawnPosition() const = 0;

    // Collision
    virtual bool InitCollisionsWithModelsSafe(const std::vector<std::string> &requiredModels) = 0;

    // Rendering specialization if needed
    virtual void RenderEditorMap() = 0;
};

#endif // ILEVEL_MANAGER_H
