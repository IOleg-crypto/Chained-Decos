// Created by I#Oleg
//

#ifndef PLAYER_H
#define PLAYER_H

#include <raylib.h>
#include <raymath.h>

#include <Model/Model.h>

// # ----------------------------------------------------------------------------
// # Player class handles the camera used to represent the player's point of view
// # ----------------------------------------------------------------------------
class Player {
private:
    Camera camera; // Raylib camera struct to represent 3D perspective
    int cameraMode; // Mode for camera(First , Free , Third , orbital)
    float moveSpeed = 3.0f; // Speed for character
public:
    Vector3 m_playerCurrentPosition;
    Vector3 m_playerLastPosition;
    Vector3 m_playerVelocity;
    float gravity = 9.8f; // Used Earth gravity as default
    float jumpStrength = 3.0f; // Adjust as needed
    bool m_isGrounded = true;
    float GroundLevel = 0;
private:
    Models modelPlayer;
public:
    Player(); // Constructor to initialize the camera and all stuff

    [[nodiscard]] Camera &getCamera(); // Returns the current camera state (read-only)
    [[nodiscard]] int &GetCameraMode();

    void SetCameraMode(int cameraMode);

    void Update(); // Updates the camera each frame (e.g., handles input and movement)
    [[nodiscard]] float GetSpeed() const; // Get character speed
    void SetSpeed(float speed);

    void Move(Vector3 offset);

    void LoadModelPlayer();

    void Jump();
};

#endif // PLAYER_H
