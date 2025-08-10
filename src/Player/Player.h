// Created by I#Oleg
//

#ifndef PLAYER_H
#define PLAYER_H

#include <memory>
#include <raylib.h>
#include <raymath.h>

#include <CameraController/CameraController.h>
#include <Collision/CollisionManager.h>
#include <Collision/CollisionSystem.h>
#include <Model/Model.h>
#include <World/Physics.h>

// Player: handles movement, camera, collisions, model
class Player
{
public:
    Player();
    ~Player();
    Player(const Player &other) = delete;
    Player(Player &&other) = delete;

    void Update();                                                        // Main update
    void UpdatePlayerBox();                                               // Update bounding box
    void UpdatePlayerCollision();                                         // Update collisions
    void ApplyGravityForPlayer(const CollisionManager &collisionManager); // Gravity + collisions

    void ApplyInput();                          // Process input
    void Move(const Vector3 &moveVector);       // Move player
    void SetPlayerPosition(const Vector3 &pos); // Set position
    void ApplyJumpImpulse(float impulse);       // Jump impulse

    std::shared_ptr<CameraController> GetCameraController() const; // Get camera

    void SetPlayerModel(Model *model);        // Set 3D model
    void ToggleModelRendering(bool useModel); // Show/hide model
    Models GetModelManager();                 // Get model manager

    [[nodiscard]] float GetSpeed();           // Get current speed
    [[nodiscard]] float GetRotationY() const; // Get Y rotation
    void SetSpeed(float speed);               // Set speed
    Vector3 GetPlayerPosition() const;        // Get position
    const Collision &GetCollision() const;    // Get collision info
    bool IsJumpCollision() const;             // Check jump collision flag

private:
    std::shared_ptr<CameraController> m_cameraController; // Camera control
    float m_cameraYaw = 0.0f;
    float m_cameraPitch = 0.0f;
    float m_cameraSmoothingFactor = 4.0f;
    Vector3 m_baseTarget = {0.0f, 2.0f, 0.0f};
    Vector3 m_originalCameraTarget = {0.0f, 2.0f, 0.0f};
    float m_rotationY = 0.0f;

    bool m_isJumping = false;
    bool m_isPlayerMoving = false;
    bool m_isJumpCollision = false;
    float m_walkSpeed = 3.0f;
    float m_runSpeed = 15.0f;
    PhysicsComponent m_physics;

    Vector3 m_playerPosition{};
    Vector3 m_lastPlayerPosition{};
    Vector3 m_playerSize{};
    Color m_playerColor = BLUE;
    BoundingBox m_playerBoundingBox{};

    Model *m_playerModel = nullptr;
    bool m_useModel = false;
    Models m_modelPlayer;

    Collision m_collision{};
    CollisionManager m_collisionManager;
};

#endif // PLAYER_H
