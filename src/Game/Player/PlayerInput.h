
#ifndef PLAYER_INPUT_H
#define PLAYER_INPUT_H

#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include "IPlayerInput.h"

// Forward declaration to break circular dependency
class Player;

// PlayerInput: handles all input-related functionality
class PlayerInput : public IPlayerInput
{
public:
    explicit PlayerInput(Player *player);

    // IPlayerInput interface implementation
    void ProcessInput() override;
    void HandleJumpInput() const override;
    void HandleEmergencyReset() const override;
    Vector3 GetInputDirection() override;
    std::pair<Vector3, Vector3> GetCameraVectors() const override;

private:
    Player *m_player;
    float m_walkSpeed;
};

#endif // PLAYER_INPUT_H
