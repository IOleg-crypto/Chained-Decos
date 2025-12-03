#ifndef ILEVEL_MANAGER_H
#define ILEVEL_MANAGER_H

#include <raylib.h>
#include <string>


/**
 * @brief Minimal LevelManager interface
 *
 * Essential API only - 7 methods (down from 20+)
 * Engine depends on this interface, not concrete LevelManager class.
 */
class ILevelManager
{
public:
    virtual ~ILevelManager() = default;

    // Map lifecycle
    virtual bool LoadMap(const std::string &path) = 0;
    virtual void UnloadMap() = 0;
    virtual bool IsMapLoaded() const = 0;

    // Map info
    virtual std::string GetCurrentMapName() const = 0;
    virtual Vector3 GetSpawnPosition() const = 0;

    // Update/Render
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
};

#endif // ILEVEL_MANAGER_H
