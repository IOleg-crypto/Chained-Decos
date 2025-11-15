//

#ifndef PLAYER_H
#define PLAYER_H

#include <memory>
#include <raylib.h>
#include <raymath.h>

#include <CameraController/Core/CameraController.h>
#include <Collision/Core/CollisionManager.h>
#include <Collision/System/CollisionSystem.h>
#include <World/Core/World.h>
#include <Model/Core/Model.h>
#include "Engine/Audio/Core/AudioManager.h"
#include "Engine/Kernel/Core/Kernel.h"

// Include component interfaces
#include "../Interfaces/IPlayerInput.h"
#include "../Interfaces/IPlayerMovement.h"
#include "../Interfaces/IPlayerMediator.h"
#include "../Components/PlayerModel.h"
#include "../Collision/PlayerCollision.h"
#include "Engine/Render/Interfaces/IGameRenderable.h"

// Forward declaration to break circular dependency
class PlayerRenderable;

// Player: main player class that uses component classes
class Player : public IPlayerMediator
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
    IGameRenderable* GetRenderable() const;
    
    
    void Update(CollisionManager& collisionManager);

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