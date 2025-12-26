#ifndef PLAYER_H
#define PLAYER_H

#include <memory>
#include <raylib.h>
#include <raymath.h>

#include "components/audio/interfaces/IAudioManager.h"
#include <components/physics/collision/core/collisionManager.h>
#include <components/physics/collision/system/collisionSystem.h>
#include <scene/camera/CameraController.h>
#include <scene/main/World.h>
#include <scene/resources/model/Model.h>

#include "IPlayerInput.h"
#include "IPlayerMovement.h"
#include "PlayerCollision.h"
#include "PlayerModel.h"
#include "PlayerRenderable.h"
#include "components/rendering/interfaces/IGameRenderable.h"
#include "core/interfaces/IPlayer.h"

// Player: main player class that uses component classes
class Player : public IPlayer
{
public:
    // Player constants - defined in .cpp file
    static Vector3 DEFAULT_SPAWN_POSITION;
    static const float MODEL_Y_OFFSET;
    static const float MODEL_SCALE;

    // DI constructor - AudioManager injected
    Player(IAudioManager *audioManager);
    ~Player();

    // Initialize services from Kernel (call after Kernel services are registered)
    void InitializeServices();

    void UpdateImpl(CollisionManager &collisionManager); // Main update
    void UpdatePlayerBox() const;                        // Update bounding box
    void UpdatePlayerCollision() const;                  // Update collisions
    void SyncCollision() const override;
    void InitializeCollision() override;

    void ApplyGravityForPlayer(CollisionManager &collisionManager); // Gravity + collisions

    // Delegate to PlayerInput
    void ApplyInput() const; // Process input

    // Delegate to PlayerMovement
    void Move(const Vector3 &moveVector) const;                // Move player
    void SetPlayerPosition(const Vector3 &pos) const override; // Set position
    void ApplyJumpImpulse(float impulse) override;             // Jump impulse
    void SnapToGroundIfNeeded(const CollisionManager &collisionManager) const;
    Vector3 StepMovement(const CollisionManager &collisionManager) const;
    void ApplyGravity(float deltaTime) const;
    void HandleEmergencyReset() const;
    void HandleJumpInput() const;

    // Camera access
    std::shared_ptr<CameraController> GetCameraController() const override; // Get camera

    // Delegate to PlayerModel
    void SetPlayerModel(Model *model) const;            // Set 3D model
    void ToggleModelRendering(bool useModel) const;     // Show/hide model
    [[nodiscard]] ModelLoader &GetModelManager() const; // Get model manager
    void SetRotationY(float rotation) const override;

    // Getters/Setters
    float GetSpeed() const override;           // Get current speed
    float GetRotationY() const override;       // Get Y rotation
    void SetSpeed(float speed) const override; // Set speed

    Vector3 GetPlayerPosition() const override;          // Get position
    Vector3 GetPlayerSize() const override;              // Get player size
    PlayerCollision &GetCollisionMutable();              // Add this method
    const Collision &GetCollision() const override;      // Get collision info
    bool IsJumpCollision() const;                        // Check jump collision flag
    BoundingBox GetPlayerBoundingBox() const override;   // Get bounding box
    const PhysicsComponent &GetPhysics() const override; // Get physics component (const)
    PhysicsComponent &GetPhysics() override;             // Get physics component (non-const)
    IPlayerMovement *GetMovement() const;
    IGameRenderable *GetRenderable() const;

    // IPlayer Interface Implementation
    Vector3 GetPosition() const override;
    void SetPosition(const Vector3 &pos) override;
    void Update(float deltaTime) override;
    Camera3D &GetCamera() override;
    void SetNoclip(bool enabled) override;
    bool IsNoclip() const override;

    void Update(CollisionManager &collisionManager);

private:
    // Component objects - using interfaces for better decoupling
    std::unique_ptr<IPlayerMovement> m_movement;
    std::unique_ptr<IPlayerInput> m_input;
    std::unique_ptr<PlayerModel> m_model;
    std::unique_ptr<PlayerCollision> m_collision;

    std::unique_ptr<PlayerRenderable> m_renderable;

    // Camera control
    std::shared_ptr<CameraController> m_cameraController;

    // Player state
    bool m_isJumping = false;
    bool m_isFallSoundPlaying = false;
    Vector3 m_boundingBoxSize{};
    Vector3 m_spawnPosition{};

    // Services - DI with raw pointers
    IAudioManager *m_audioManager;
    std::shared_ptr<CollisionManager> m_collisionManager;
};

#endif // PLAYER_H
