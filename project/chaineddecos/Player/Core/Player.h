#ifndef PLAYER_H
#define PLAYER_H

#include <memory>
#include <raylib.h>
#include <raymath.h>

#include "components/audio/Core/AudioManager.h"
#include <components/physics/collision/Core/CollisionManager.h>
#include <components/physics/collision/System/CollisionSystem.h>
#include <scene/3d/camera/Core/CameraController.h>
#include <scene/main/Core/World.h>
#include <scene/resources/model/Core/Model.h>


// Include component interfaces
#include "../Collision/PlayerCollision.h"
#include "../Components/PlayerModel.h"
#include "../Interfaces/IPlayerInput.h"

#include "../Interfaces/IPlayerMovement.h"
#include "core/interfaces/IPlayer.h"
#include "servers/rendering/Interfaces/IGameRenderable.h"


// Forward declaration to break circular dependency
class PlayerRenderable;

// Player: main player class that uses component classes
class Player : public IPlayer
{
public:
    // Player constants - defined in .cpp file
    static Vector3 DEFAULT_SPAWN_POSITION;
    static const float MODEL_Y_OFFSET;
    static const float MODEL_SCALE;

    // Constructors and methods
    Player();
    ~Player();

    // Initialize services from Kernel (call after Kernel services are registered)
    void InitializeServices();

    void UpdateImpl(CollisionManager &collisionManager); // Main update
    void UpdatePlayerBox() const;                        // Update bounding box
    void UpdatePlayerCollision() const;                  // Update collisions
    void SyncCollision() const;
    void InitializeCollision() override;

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
    void SetPlayerModel(Model *model) const;            // Set 3D model
    void ToggleModelRendering(bool useModel) const;     // Show/hide model
    [[nodiscard]] ModelLoader &GetModelManager() const; // Get model manager
    void SetRotationY(float rotation) const override;

    // Getters/Setters
    float GetSpeed() const override;           // Get current speed
    float GetRotationY() const override;       // Get Y rotation
    void SetSpeed(float speed) const override; // Set speed

    Vector3 GetPlayerPosition() const;                // Get position
    Vector3 GetPlayerSize() const;                    // Get player size
    PlayerCollision &GetCollisionMutable();           // Add this method
    const Collision &GetCollision() const;            // Get collision info
    bool IsJumpCollision() const;                     // Check jump collision flag
    BoundingBox GetPlayerBoundingBox() const;         // Get bounding box
    const LegacyPhysicsComponent &GetPhysics() const; // Get physics component (const)
    LegacyPhysicsComponent &GetPhysics();             // Get physics component (non-const)
    IPlayerMovement *GetMovement() const;
    IGameRenderable *GetRenderable() const;

    // IPlayer Interface Implementation
    Vector3 GetPosition() const override
    {
        return GetPlayerPosition();
    }
    void SetPosition(const Vector3 &pos) override
    {
        SetPlayerPosition(pos);
    }
    void Update(float deltaTime) override;
    Camera3D &GetCamera() override;
    void SetNoclip(bool enabled) override;
    bool IsNoclip() const override;

    // Service injection
    void SetAudioManager(std::shared_ptr<AudioManager> audioManager);

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

    // Services from Kernel (cached)
    std::shared_ptr<AudioManager> m_audioManager;
    std::shared_ptr<CollisionManager> m_collisionManager;
};

#endif // PLAYER_H