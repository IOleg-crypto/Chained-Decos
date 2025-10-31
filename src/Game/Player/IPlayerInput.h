#ifndef IPLAYERINPUT_H
#define IPLAYERINPUT_H

#include <raylib.h>
#include <raymath.h>
#include <utility>

// Forward declaration to break circular dependency
class Player;

class IPlayerInput
{
public:
    virtual ~IPlayerInput() = default;

    // Process input and apply movement
    virtual void ProcessInput() = 0;

    // Handle jump input
    virtual void HandleJumpInput() const = 0;

    // Handle emergency reset
    virtual void HandleEmergencyReset() const = 0;

    // Get input direction vector
    virtual Vector3 GetInputDirection() = 0;

    // Get camera vectors (forward and right)
    virtual std::pair<Vector3, Vector3> GetCameraVectors() const = 0;
};

#endif // IPLAYERINPUT_H

