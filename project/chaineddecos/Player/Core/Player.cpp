#include "Player.h"
#include "../Collision/PlayerCollision.h"
#include "../Components/PlayerInput.h"
#include "../Components/PlayerModel.h"
#include "../Components/PlayerMovement.h"
#include "../Components/PlayerRenderable.h"
#include <memory>
#include <raylib.h>
#include <scene/3d/camera/Core/CameraController.h>
#include <scene/main/Core/World.h>
#include <servers/rendering/Interfaces/IGameRenderable.h>

// Define player constants
Vector3 Player::DEFAULT_SPAWN_POSITION = {0.0f, 0.0f, 0.0f}; // Safe spawn position above ground
const float Player::MODEL_Y_OFFSET = -1.f;

Player::Player() : m_cameraController(std::make_shared<CameraController>())
{
    // TODO: Get services from Kernel once service registration is complete (Task 14)
    // For now, services will be set externally or remain null until properly initialized

    m_boundingBoxSize = {1.2f, 2.8f, 1.2f};

    // Create component objects
    m_movement = std::make_unique<PlayerMovement>(this);
    m_input = std::make_unique<PlayerInput>(this);
    m_model = std::make_unique<PlayerModel>();
    m_collision = std::make_unique<PlayerCollision>(this);
    m_renderable = std::make_unique<PlayerRenderable>(this);

    // Initialize physics - start ungrounded so gravity can act
    m_movement->GetPhysics().SetGroundLevel(false);
    m_movement->GetPhysics().SetVelocity({0.0f, 0.0f, 0.0f});

    // Update collision system
    UpdatePlayerBox();
    UpdatePlayerCollision();
}

Player::~Player() = default;

void Player::SetAudioManager(std::shared_ptr<AudioManager> audioManager)
{
    m_audioManager = audioManager;
}

void Player::InitializeServices()
{
    TraceLog(LOG_INFO, "[Player] InitializeServices called (AudioManager will be set externally)");
}

// Main update function called every frame
void Player::UpdateImpl(CollisionManager &collisionManager)
{
    // Process input first
    m_input->ProcessInput();

    // Update audio looping sounds
    if (m_audioManager)
    {
        m_audioManager->UpdateLoopingSounds();
    }

    // Update camera
    m_cameraController->UpdateCameraRotation();
    m_cameraController->UpdateMouseRotation(m_cameraController->GetCamera(),
                                            m_movement->GetPosition());
    m_cameraController->Update();

    // Apply physics
    float deltaTime = IsWindowReady() ? GetFrameTime() : (1.0f / 60.0f);
    m_movement->SetCollisionManager(&collisionManager);

    HandleJumpInput();
    HandleEmergencyReset();

    if (!m_movement->IsNoclip())
    {
        m_movement->ApplyGravity(deltaTime);

        // Integrate horizontal velocity from physics into desired position
        Vector3 horizVel = m_movement->GetPhysics().GetVelocity();
        horizVel.y = 0.0f;
        if (Vector3Length(horizVel) > 0.0f)
        {
            Vector3 step = Vector3Scale(horizVel, deltaTime);
            m_movement->Move(step);
        }

        Vector3 newPosition = m_movement->StepMovement(collisionManager);

        SetPlayerPosition(newPosition);

        UpdatePlayerBox();
        UpdatePlayerCollision();

        // Only snap when falling slowly to avoid oscillation
        if (!m_movement->GetPhysics().IsGrounded() &&
            m_movement->GetPhysics().GetVelocity().y <= 0.0f)
        {
            m_movement->SnapToGround(collisionManager);

            // Apply screen shake during fall (intensity based on fall speed)
            float fallSpeed = abs(m_movement->GetPhysics().GetVelocity().y);
            if (fallSpeed > 5.0f) // Only shake if falling fast enough
            {
                // Normalize fall speed (0-60 range -> 0-1 range, then scale to shake intensity)
                float normalizedFall = std::min(fallSpeed / 60.0f, 1.0f);
                float shakeIntensity = normalizedFall * 0.15f; // Max 0.15 intensity
                float shakeDuration = 0.3f;                    // Short continuous shake during fall
                m_cameraController->AddScreenShake(shakeIntensity, shakeDuration);
            }
        }

        // Don't force ground state based on height alone - rely on collision detection
        // This allows player to fall properly when there's no ground below
        // The UpdateGrounded() method handles ground detection based on actual collisions

        // Track falling state for screen shake and landing impact
        static bool wasFalling = false;
        static float lastFallSpeed = 0.0f;

        if (m_movement->GetPhysics().IsGrounded())
        {
            // Check if we just landed (was falling, now grounded)
            TraceLog(LOG_DEBUG, "[Player] Grounded check: wasFalling=%d, lastFallSpeed=%.2f",
                     wasFalling, lastFallSpeed);
            if (wasFalling)
            {
                TraceLog(LOG_INFO, "[Player] Landed with fall speed: %.2f", lastFallSpeed);
                // Stop the looping fall sound
                if (m_audioManager && m_isFallSoundPlaying)
                {
                    m_audioManager->StopLoopingSoundEffect("player_fall");
                    m_isFallSoundPlaying = false;
                    TraceLog(LOG_INFO, "[Player] Stopped looping fall sound on landing");
                }
            }
            if (wasFalling)
            {
                TraceLog(LOG_INFO, "[Player] Landing impact detected, playing fall sound");
                // Strong impact shake on landing
                float normalizedFall = std::min(lastFallSpeed / 60.0f, 1.0f);
                float impactIntensity = normalizedFall * 0.3f; // Max 0.3 intensity
                float impactDuration = 0.4f;
                m_cameraController->AddScreenShake(impactIntensity, impactDuration);
                if (m_audioManager)
                {
                    TraceLog(LOG_INFO,
                             "[Player] AudioManager available, playing player_fall sound");
                    m_audioManager->PlaySoundEffect("player_fall", 1.0f);
                }
                else
                {
                    TraceLog(LOG_WARNING, "[Player] AudioManager is null, cannot play fall sound");
                }
            }

            wasFalling = false;
            lastFallSpeed = 0.0f;
            m_isJumping = false;

            // Ensure velocity is zero when grounded to prevent sliding
            Vector3 currentVel = m_movement->GetPhysics().GetVelocity();
            if (fabsf(currentVel.y) < 0.1f)
            {
                currentVel.y = 0.0f;
                m_movement->GetPhysics().SetVelocity(currentVel);
            }
        }
        else
        {
            // Track falling state for landing impact
            wasFalling = true;
            lastFallSpeed = abs(m_movement->GetPhysics().GetVelocity().y);
            TraceLog(LOG_DEBUG, "[Player] Falling: velocity.y=%.2f, lastFallSpeed=%.2f",
                     m_movement->GetPhysics().GetVelocity().y, lastFallSpeed);

            // Handle continuous fall sound
            if (m_audioManager)
            {
                if (!m_isFallSoundPlaying && lastFallSpeed > 2.0f)
                {
                    // Start looping fall sound
                    m_audioManager->PlayLoopingSoundEffect("player_fall",
                                                           6.5f); // Lower volume for continuous
                    m_isFallSoundPlaying = true;
                    TraceLog(LOG_INFO, "[Player] Started looping fall sound");
                }
                else if (m_isFallSoundPlaying && lastFallSpeed < 1.0f)
                {
                    // Stop looping fall sound when fall speed is low
                    m_audioManager->StopLoopingSoundEffect("player_fall");
                    m_isFallSoundPlaying = false;
                    TraceLog(LOG_INFO, "[Player] Stopped looping fall sound due to low fall speed");
                }
            }
        }
    }
    else
    {
        // In noclip mode, just update position based on velocity without gravity or collisions
        Vector3 vel = m_movement->GetPhysics().GetVelocity();
        Vector3 newPosition = m_movement->GetPosition();
        newPosition.x += vel.x * deltaTime;
        newPosition.y += vel.y * deltaTime;
        newPosition.z += vel.z * deltaTime;
        m_movement->SetPosition(newPosition);
        SetPlayerPosition(newPosition);
        UpdatePlayerBox();
        UpdatePlayerCollision();
    }
}

float Player::GetSpeed() const
{
    return m_movement->GetSpeed();
}

float Player::GetRotationY() const
{
    return m_movement->GetRotationY();
}

void Player::SetSpeed(const float speed) const
{
    m_movement->SetSpeed(speed);
}

void Player::Move(const Vector3 &moveVector) const
{
    m_movement->Move(moveVector);
}

// Handle input both on ground and mid-air
void Player::ApplyInput() const
{
    m_input->ProcessInput();
}

std::shared_ptr<CameraController> Player::GetCameraController() const
{
    return m_cameraController;
}

ModelLoader &Player::GetModelManager() const
{
    return m_model->GetModelManager();
}

void Player::SetPlayerModel(Model *model) const
{
    m_model->SetModel(model);
}

void Player::UpdatePlayerBox() const
{
    m_collision->UpdateBoundingBox();
}

void Player::UpdatePlayerCollision() const
{
    m_collision->Update();
}

void Player::SyncCollision() const
{
    UpdatePlayerBox();
    UpdatePlayerCollision();
}

void Player::ToggleModelRendering(const bool useModel) const
{
    m_model->ToggleModelRendering(useModel);
}

void Player::SetPlayerPosition(const Vector3 &pos) const
{
    m_movement->SetPosition(pos);
}

const Collision &Player::GetCollision() const
{
    return *m_collision;
}

bool Player::IsJumpCollision() const
{
    return m_collision->IsJumpCollision();
}

Vector3 Player::GetPlayerPosition() const
{
    return m_movement->GetPosition();
}

Vector3 Player::GetPlayerSize() const
{
    return m_boundingBoxSize;
}

// Apply jump impulse based on mass and direction
void Player::ApplyJumpImpulse(float impulse)
{
    // Delegate jump gating (grounded/coyote-time) to PlayerMovement
    m_movement->ApplyJumpImpulse(impulse);
    if (m_movement->GetPhysics().GetVelocity().y > 0.0f)
        m_isJumping = true;
}

void Player::ApplyGravityForPlayer(CollisionManager &collisionManager)
{
    // Legacy function - now delegates to Update()
    UpdateImpl(collisionManager);
}

void Player::HandleJumpInput() const
{
    m_input->HandleJumpInput();
}

void Player::HandleEmergencyReset() const
{
    m_input->HandleEmergencyReset();
}

void Player::ApplyGravity(const float deltaTime) const
{
    m_movement->ApplyGravity(deltaTime);
}

Vector3 Player::StepMovement(const CollisionManager &collisionManager) const
{
    return m_movement->StepMovement(collisionManager);
}

void Player::SnapToGroundIfNeeded(const CollisionManager &collisionManager) const
{
    m_movement->SnapToGround(collisionManager);
}

BoundingBox Player::GetPlayerBoundingBox() const
{
    return m_collision->GetBoundingBox();
}

const LegacyPhysicsComponent &Player::GetPhysics() const
{
    return m_movement->GetPhysics();
}

LegacyPhysicsComponent &Player::GetPhysics()
{
    return m_movement->GetPhysics();
}

void Player::SetRotationY(const float rotationY) const
{
    m_movement->SetRotationY(rotationY);
}

IPlayerMovement *Player::GetMovement() const
{
    return m_movement.get();
}
PlayerCollision &Player::GetCollisionMutable()
{
    return *m_collision;
}

// IGameRenderable access
IGameRenderable *Player::GetRenderable() const
{
    return m_renderable.get();
}

void Player::Update(CollisionManager &collisionManager)
{
    UpdateImpl(collisionManager);
}

void Player::Update(float deltaTime)
{
    if (m_collisionManager) {
        Update(*m_collisionManager);
    }
}

Camera3D& Player::GetCamera()
{
    return m_cameraController->GetCamera();
}
