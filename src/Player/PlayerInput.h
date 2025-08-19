
#ifndef PLAYER_INPUT_H
#define PLAYER_INPUT_H

#include <raylib.h>
#include <raymath.h>
#include <iostream>

// Forward declarations
class Player;

// PlayerInput: handles all input-related functionality
class PlayerInput
{
public:
    PlayerInput(Player* player);
    ~PlayerInput() = default;
    
    // Process input and apply movement
    void ProcessInput();
    
    // Handle jump input
    void HandleJumpInput();
    
    // Handle emergency reset
    void HandleEmergencyReset();
    
    // Get input direction vector
    Vector3 GetInputDirection();
    
    // Get camera vectors (forward and right)
    std::pair<Vector3, Vector3> GetCameraVectors() const;
    
private:
    Player* m_player;
};

#endif // PLAYER_INPUT_H


