//
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
#include <Player/PositionData.h>
#include <World/Physics.h>

//
// Player class handles the camera used to represent the player's point of view
//
class Player
{
    // -------------------- Camera & View --------------------
  private:
    std::shared_ptr<CameraController> m_cameraController;
    float m_cameraYaw = 0.0f;
    float m_cameraPitch = 0.0f;
    float m_cameraSmoothingFactor = 4.0f;
    Vector3 m_baseTarget = {0.0f, 2.0f, 0.0f};
    Vector3 m_originalCameraTarget = {0.0f, 2.0f, 0.0f};

    // -------------------- Movement & Physics --------------------
  private:
    float m_walkSpeed = 3.0f;
    float m_runSpeed = 15.0f;
    float m_jumpStrength = 8.0f;
    float m_jumpOffsetY = 0.0f;
    bool m_isJumping = false;
    bool m_isPlayerMoving = false;
    PhysicsComponent m_physics;
    PositionData m_posData;
    bool m_isJumpCollision = false;

    // -------------------- Player State --------------------
  private:
    Vector3 m_playerPosition{};
    Vector3 m_playerSize{};
    Color m_playerColor = BLUE;
    BoundingBox m_playerBoundingBox{};

    // -------------------- Model --------------------
  private:
    Model *m_playerModel = nullptr;
    bool m_useModel = false;
    Models m_modelPlayer;

    // -------------------- Collision --------------------
  private:
    Collision m_collision{};
    CollisionManager m_collisionManager;

    // -------------------- Public Interface --------------------
  public:
    Player(); // Constructor
    ~Player();
    Player(const Player &other) = delete;
    Player(Player &&other) = delete;

    // Update Methods
    void Update(); // Main update logic
    void UpdatePositionHistory();
    void UpdatePlayerBox();

    // Movement
    void ApplyInput();
    void Move(const Vector3 &moveVector);
    [[deprecated("Another function do this - ApplyGravityForPlayer")]] void Jump();
    void SetPlayerPosition(const Vector3 &pos);
    void UpdateMouseRotation() const;

    // Camera
    std::shared_ptr<CameraController> GetCameraController() const;

    // Model
    void SetPlayerModel(Model *model);
    void ToggleModelRendering(bool useModel);
    Models GetModelManager();

    // Update player collision
    void UpdateCollision();

    // Getters
    [[nodiscard]] float GetSpeed();
    void SetSpeed(float speed);
    Vector3 GetPlayerPosition() const;
    PositionData GetPlayerData() const;
    const Collision &GetCollision() const;
    // Collision
    bool IsJumpCollision() const;
    void ApplyGravityForPlayer(const CollisionManager &collisionManager);
    Matrix GetPlayerRotation() const;
};

#endif // PLAYER_H
