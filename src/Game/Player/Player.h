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
#include <Render/IRenderable.h>

// Include component interfaces
#include "IPlayerInput.h"
#include "IPlayerMovement.h"
#include "PlayerInput.h"
#include "PlayerMovement.h"
#include "PlayerCollision.h"
#include "PlayerModel.h"

// Player: main player class that uses component classes
class Player : public IRenderable
{
public:
    // Player constants - defined in .cpp file
    static const Vector3 DEFAULT_SPAWN_POSITION;
    static const float MODEL_Y_OFFSET;
    static const float MODEL_SCALE;

    // Constructors and methods
    Player();
    ~Player();

    void UpdateImpl(CollisionManager &collisionManager); // Main update
    void UpdatePlayerBox() const;                          // Update bounding box
    void UpdatePlayerCollision() const;                    // Update collisions
    void SyncCollision() const;

    void ApplyGravityForPlayer(CollisionManager &collisionManager); // Gravity + collisions

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
    IPlayerMovement *GetMovement() const;

    // IRenderable interface implementations
    void Update(CollisionManager& collisionManager) override;
    void Render() override;
    Vector3 GetPosition() const override;
    BoundingBox GetBoundingBox() const override;
    void UpdateCollision() override;
    Camera GetCamera() const override;
    bool IsGrounded() const override;

private:
    // Component objects - using interfaces for better decoupling
    std::unique_ptr<IPlayerMovement> m_movement;
    std::unique_ptr<IPlayerInput> m_input;
    std::unique_ptr<PlayerModel> m_model;
    std::unique_ptr<PlayerCollision> m_collision;

    // Camera control
    std::shared_ptr<CameraController> m_cameraController;

    // Player state
    bool m_isJumping = false;
    Vector3 m_boundingBoxSize{};
};

#endif // PLAYER_H