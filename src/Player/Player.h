// Created by I#Oleg
//

#ifndef PLAYER_H
#define PLAYER_H

#include <raylib.h>
#include <raymath.h>

#include <Model/Model.h>
#include <CameraController/CameraController.h>
#include <Player/PositionData.h>
#include <World/PhysicsData.h>

// # ----------------------------------------------------------------------------
// # Player class handles the camera used to represent the player's point of view
// # ----------------------------------------------------------------------------
class Player {
private:
    float walkSpeed = 3.0f; // Speed for character
    float runSpeed = 15;
    float jumpStrength = 3.0f; // Adjust as needed
    PositionData posData;
    PhysicsData physData;
private:
    Models modelPlayer;
    CameraController *cameraController;
public:
    Player(); // Constructor to initialize the camera and all stuff
    void Update(); // Updates the camera each frame (e.g., handles input and movement)
    [[nodiscard]] float GetSpeed(); // Get character speed
    void SetSpeed(float speed);
    // Move player (camera) in 3D
    void Move(Vector3 offset) const;
    // Loads model player
    void LoadModelPlayer();
    // Allow player jumps
    void Jump();
    // Take history of player position(needed for player jump)
    void PositionHistory();
    // Allows W,A,S,D - movement
    void ApplyInput();
    // Get camera
    [[nodiscard]] CameraController *getCameraController() const;
    // Get model manager
    Models getModelManager();
};

#endif // PLAYER_H
