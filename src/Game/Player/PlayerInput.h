
#ifndef PLAYER_INPUT_H
#define PLAYER_INPUT_H

#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include "Player.h"

// PlayerInput: handles all input-related functionality
class PlayerInput
{
public:
    PlayerInput(Player *player);

    // Process input and apply movement
    void ProcessInput();

    // Handle jump input
    void HandleJumpInput() const;

    // Handle emergency reset
    void HandleEmergencyReset() const;

    // Get input direction vector
    Vector3 GetInputDirection();

    // Get camera vectors (forward and right)
    std::pair<Vector3, Vector3> GetCameraVectors() const;

private:
    Player *m_player;
    float m_walkSpeed;
};

#endif // PLAYER_INPUT_H
