#include "Player.h"
#include "PlayerRenderable.h"
#include "PlayerMovement.h"
#include "PlayerInput.h"
#include "PlayerCollision.h"
#include "PlayerModel.h"
#include <Render/IRenderable.h>
#include <CameraController/CameraController.h>
#include <World/World.h>
#include <memory>
#include <raylib.h>

// Define player constants
const Vector3 Player::DEFAULT_SPAWN_POSITION = {0.0f, 160.0f,
                                                0.0f}; // Safe spawn position above ground
const float Player::MODEL_Y_OFFSET = -1.f;

Player::Player() : m_cameraController(std::make_shared<CameraController>())
{
    // Initialize player bounding box size
    m_boundingBoxSize = {1.0f, 2.5f, 1.0f};

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

// Main update function called every frame
void Player::UpdateImpl(CollisionManager &collisionManager)
{
    // Process input first
    m_input->ProcessInput();

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
        if (!m_movement->GetPhysics().IsGrounded() && m_movement->GetPhysics().GetVelocity().y <= 0.0f)
        {
            m_movement->SnapToGround(collisionManager);
        }

        // Don't force ground state based on height alone - rely on collision detection
        // This allows player to fall properly when there's no ground below
        // The UpdateGrounded() method handles ground detection based on actual collisions

        if (m_movement->GetPhysics().IsGrounded())
        {
            m_isJumping = false;
            // Ensure velocity is zero when grounded to prevent sliding
            Vector3 currentVel = m_movement->GetPhysics().GetVelocity();
            if (fabsf(currentVel.y) < 0.1f)
            {
                currentVel.y = 0.0f;
                m_movement->GetPhysics().SetVelocity(currentVel);
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

float Player::GetSpeed() const { return m_movement->GetSpeed(); }

float Player::GetRotationY() const { return m_movement->GetRotationY(); }

void Player::SetSpeed(const float speed) const { m_movement->SetSpeed(speed); }

void Player::Move(const Vector3 &moveVector) const { m_movement->Move(moveVector); }

// Handle input both on ground and mid-air
void Player::ApplyInput() const { m_input->ProcessInput(); }

std::shared_ptr<CameraController> Player::GetCameraController() const { return m_cameraController; }

ModelLoader &Player::GetModelManager() const { return m_model->GetModelManager(); }

void Player::SetPlayerModel(Model *model) const { m_model->SetModel(model); }

void Player::UpdatePlayerBox() const { m_collision->UpdateBoundingBox(); }

void Player::UpdatePlayerCollision() const { m_collision->Update(); }

void Player::SyncCollision() const {
    UpdatePlayerBox();
    UpdatePlayerCollision();
}

void Player::ToggleModelRendering(const bool useModel) const
{
    m_model->ToggleModelRendering(useModel);
}

void Player::SetPlayerPosition(const Vector3 &pos) const { m_movement->SetPosition(pos); }

const Collision &Player::GetCollision() const { return *m_collision; }

bool Player::IsJumpCollision() const { return m_collision->IsJumpCollision(); }

Vector3 Player::GetPlayerPosition() const { return m_movement->GetPosition(); }

Vector3 Player::GetPlayerSize() const { return m_boundingBoxSize; }

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

void Player::HandleJumpInput() const { m_input->HandleJumpInput(); }

void Player::HandleEmergencyReset() const { m_input->HandleEmergencyReset(); }

void Player::ApplyGravity(const float deltaTime) const { m_movement->ApplyGravity(deltaTime); }

Vector3 Player::StepMovement(const CollisionManager &collisionManager) const
{
    return m_movement->StepMovement(collisionManager);
}

void Player::SnapToGroundIfNeeded(const CollisionManager &collisionManager) const
{
    m_movement->SnapToGround(collisionManager);
}

BoundingBox Player::GetPlayerBoundingBox() const { return m_collision->GetBoundingBox(); }

const PhysicsComponent &Player::GetPhysics() const { return m_movement->GetPhysics(); }

PhysicsComponent &Player::GetPhysics() { return m_movement->GetPhysics(); }

void Player::SetRotationY(const float rotationY) const { m_movement->SetRotationY(rotationY); }

IPlayerMovement *Player::GetMovement() const { return m_movement.get(); }
PlayerCollision& Player::GetCollisionMutable() {
    return *m_collision;
}

// IRenderable access
IRenderable* Player::GetRenderable() const {
    return m_renderable.get();
}

// Update method для сумісності (делегує до UpdateImpl)
void Player::Update(CollisionManager& collisionManager) {
    UpdateImpl(collisionManager);
}

