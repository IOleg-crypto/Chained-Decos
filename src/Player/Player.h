// Created by I#Oleg
//

#ifndef PLAYER_H
#define PLAYER_H

#include <raylib.h>
#include <raymath.h>

#include <Model/Model.h>
#include <CameraController/CameraController.h>

// # ----------------------------------------------------------------------------
// # Player class handles the camera used to represent the player's point of view
// # ----------------------------------------------------------------------------
class Player {
private:
    float walkSpeed = 3.0f; // Speed for character
    float runSpeed = 15;
    Vector3 m_playerCurrentPosition;
    Vector3 m_playerLastPosition;
    Vector3 m_playerVelocity;
    float gravity = 10.0f; // Used Earth gravity as default
    float jumpStrength = 3.0f; // Adjust as needed
    float velocityY = 0.0;
    bool m_isGrounded = true;
    float GroundLevel = 5.f; // if lower , use see player under world
    float dt = 0; // FrameRate Time
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
};

#endif // PLAYER_H
