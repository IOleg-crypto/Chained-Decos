//

#ifndef PLAYER_H
#define PLAYER_H

#include <memory>
#include <raylib.h>
#include <raymath.h>

#include <CameraController/CameraController.h>
#include <Collision/CollisionManager.h>
#include <Collision/CollisionSystem.h>
#include <World/World.h>
#include <Model/Model.h>

// Include new component headers
#include "PlayerCollision.h"
#include "PlayerInput.h"
#include "PlayerModel.h"
#include "PlayerMovement.h"

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

    void Update(const CollisionManager &collisionManager); // Main update
    void UpdatePlayerBox() const;                          // Update bounding box
    void UpdatePlayerCollision() const;                    // Update collisions
    void SyncCollision() const;

    void ApplyGravityForPlayer(const CollisionManager &collisionManager); // Gravity + collisions

    // Delegate to PlayerInput
    void ApplyInput() const; // Process input

    // Delegate to PlayerMovement
    void Move(const Vector3 &moveVector) const;       // Move player
    void SetPlayerPosition(const Vector3 &pos) const; // Set position
    void ApplyJumpImpulse(float impulse);             // Jump impulse
    void SnapToGroundIfNeeded(const CollisionManager &collisionManager) const;
    Vector3 StepMovement(const CollisionManager &collisionManager) const;
    void ApplyGravity(float deltaTime) const;
    void HandleEmergencyReset() const;
    void HandleJumpInput() const;

    // Camera access
    std::shared_ptr<CameraController> GetCameraController() const; // Get camera

    // Delegate to PlayerModel
    void SetPlayerModel(Model *model) const;        // Set 3D model
    void ToggleModelRendering(bool useModel) const; // Show/hide model
    [[nodiscard]] ModelLoader &GetModelManager() const;                // Get model manager
    void SetRotationY(float rotationY) const;

    // Getters/Setters
    [[nodiscard]] float GetSpeed() const;             // Get current speed
    [[nodiscard]] float GetRotationY() const;   // Get Y rotation
    void SetSpeed(float speed) const;           // Set speed
    Vector3 GetPlayerPosition() const;          // Get position
    Vector3 GetPlayerSize() const;              // Get player size
    PlayerCollision &GetCollisionMutable();           // Add this method
    const Collision &GetCollision() const;      // Get collision info
    bool IsJumpCollision() const;               // Check jump collision flag
    BoundingBox GetPlayerBoundingBox() const;   // Get bounding box
    const PhysicsComponent &GetPhysics() const; // Get physics component (const)
    PhysicsComponent &GetPhysics();             // Get physics component (non-const)
    PlayerMovement *GetMovement() const;

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