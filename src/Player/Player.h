// Created by I#Oleg
//

#ifndef PLAYER_H
#define PLAYER_H

#include <raylib.h>
#include <raymath.h>

// Player class handles the camera used to represent the player's point of view
class Player {
private:
    Camera camera; // Raylib camera struct to represent 3D perspective
    int cameraMode; // Mode for camera(First , Free , Third , orbital)
    float moveSpeed = 0.1f; // Speed for character
public:
    Player(); // Constructor to initialize the camera

    [[nodiscard]] Camera &getCamera(); // Returns the current camera state (read-only)
    [[nodiscard]] int &GetCameraMode();
    void SetCameraMode(int cameraMode);

    void Update(); // Updates the camera each frame (e.g., handles input and movement)
    float GetSpeed() const; // Get character speed
    void SetSpeed(float speed);
    void Move(Vector3 offset);


};

#endif // PLAYER_H
