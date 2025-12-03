#ifndef IWORLD_MANAGER_H
#define IWORLD_MANAGER_H

#include <raylib.h>

/**
 * @brief Interface for World Management
 *
 * Lightweight interface to avoid including heavy World.h with all dependencies
 */
class IWorldManager
{
public:
    virtual ~IWorldManager() = default;

    // World state queries
    virtual bool IsPointInWorld(const Vector3 &point) const = 0;
    virtual bool IsPointOnGround(const Vector3 &point) const = 0;
    virtual float GetGroundHeight() const = 0;

    // Lifecycle
    virtual void Update(float deltaTime) = 0;
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
};

#endif // IWORLD_MANAGER_H
