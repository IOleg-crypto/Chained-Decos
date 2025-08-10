//
//  Created by I#Oleg
//
#ifndef PLAYER_H
#define PLAYER_H

#include <memory>
#include <raylib.h>
#include <raymath.h>

#include <CameraController/CameraController.h>
#include <Collision/CollisionManager.h>
#include <Model/Model.h>
#include <World/Physics.h>

// Player handles movement, camera, collisions, and optional model rendering.
class Player
{
public:
    Player();
    ~Player();

    void Update();                  // Main per-frame update
    void ApplyInput();              // Handle keyboard input
    void Move(const Vector3& move); // Apply movement vector
    void ApplyJumpImpulse(float impulse);

    void SetPlayerPosition(const Vector3& pos);
    Vector3 GetPlayerPosition() const { return m_PlayerPosition; }

    // Camera
    std::shared_ptr<CameraController> GetCameraController() const { return m_CameraController; }

    // Model
    void SetPlayerModel(Model* model);
    void ToggleModelRendering(bool useModel) { m_UseModel = useModel; }

    // Collision
    void UpdatePlayerBox();                           // Update bounding box
    void UpdatePlayerCollision();                     // Check collisions
    void ApplyGravityForPlayer(const CollisionManager& collisionManager);
    bool IsJumpCollision() const { return m_IsJumpCollision; }

    float GetRotationY() const { return m_RotationY; }

private:
    // --- Camera ---
    std::shared_ptr<CameraController> m_CameraController;
    float m_CameraYaw = 0.0f;
    float m_CameraPitch = 0.0f;
    float m_CameraSmoothingFactor = 4.0f;
    Vector3 m_BaseTarget = { 0.0f, 2.0f, 0.0f };
    Vector3 m_OriginalCameraTarget = { 0.0f, 2.0f, 0.0f };
    float m_RotationY = 0.0f; // Y-axis rotation

    // --- Movement / Physics ---
    bool m_IsJumping = false;
    bool m_IsPlayerMoving = false;
    bool m_IsJumpCollision = false;
    float m_WalkSpeed = 3.0f;
    float m_RunSpeed = 15.0f;
    PhysicsComponent m_Physics;

    // --- State ---
    Vector3 m_PlayerPosition{};
    Vector3 m_LastPlayerPosition{};
    Vector3 m_PlayerSize{};
    Color m_PlayerColor = BLUE;
    BoundingBox m_PlayerBoundingBox{};

    // --- Model ---
    Model* m_PlayerModel = nullptr;
    bool m_UseModel = false;
    Models m_ModelPlayer;

    // --- Collision ---
    Collision m_Collision{};
    CollisionManager m_CollisionManager;
};

#endif
