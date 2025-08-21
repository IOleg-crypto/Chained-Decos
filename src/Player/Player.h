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

// Include new component headers
#include <Player/PlayerCollision.h>
#include <Player/PlayerInput.h>
#include <Player/PlayerModel.h>
#include <Player/PlayerMovement.h>

// Player: main player class that uses component classes
class Player
{
public:
    // Player constants - defined in .cpp file
    static const Vector3 DEFAULT_SPAWN_POSITION;
    static const float MODEL_Y_OFFSET;
    static const float MODEL_SCALE;

    // Constructors and methods
    Player();
    ~Player();

    void Update(const CollisionManager &collisionManager);                // Main update
    void UpdatePlayerBox();                                               // Update bounding box
    void UpdatePlayerCollision();                                         // Update collisions
    void ApplyGravityForPlayer(const CollisionManager &collisionManager); // Gravity + collisions

    // Delegate to PlayerInput
    void ApplyInput(); // Process input

    // Delegate to PlayerMovement
    void Move(const Vector3 &moveVector);       // Move player
    void SetPlayerPosition(const Vector3 &pos); // Set position
    void ApplyJumpImpulse(float impulse);       // Jump impulse
    void SnapToGroundIfNeeded(const CollisionManager &collisionManager);
    Vector3 StepMovement(const CollisionManager &collisionManager);
    void ApplyGravity(float deltaTime);
    void HandleEmergencyReset();
    void HandleJumpInput();

    // Camera access
    std::shared_ptr<CameraController> GetCameraController() const; // Get camera

    // Delegate to PlayerModel
    void SetPlayerModel(Model *model);        // Set 3D model
    void ToggleModelRendering(bool useModel); // Show/hide model
    Models &GetModelManager();                // Get model manager
    void SetRotationY(float rotationY);

    // Getters/Setters
    [[nodiscard]] float GetSpeed();             // Get current speed
    [[nodiscard]] float GetRotationY() const;   // Get Y rotation
    void SetSpeed(float speed);                 // Set speed
    Vector3 GetPlayerPosition() const;          // Get position
    Vector3 GetPlayerSize() const;              // Get player size
    const Collision &GetCollision() const;      // Get collision info
    bool IsJumpCollision() const;               // Check jump collision flag
    BoundingBox GetPlayerBoundingBox() const;   // Get bounding box
    const PhysicsComponent &GetPhysics() const; // Get physics component (const)
    PhysicsComponent &GetPhysics();             // Get physics component (non-const)

private:
    // Component objects
    std::unique_ptr<PlayerMovement> m_movement;
    std::unique_ptr<PlayerInput> m_input;
    std::unique_ptr<PlayerModel> m_model;
    std::unique_ptr<PlayerCollision> m_collision;

    // Camera control
    std::shared_ptr<CameraController> m_cameraController;

    // Player state
    bool m_isJumping = false;
    Vector3 m_playerSize{};
};

#endif // PLAYER_H