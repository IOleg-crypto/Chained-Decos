//
// Created by I#Oleg
//
#include "Player.h"
#include "PositionData.h"
#include "raylib.h"
#include <memory>
#include <string>

Player::Player() : m_cameraController(std::make_shared<CameraController>())
{
    m_originalCameraTarget = m_cameraController->GetCamera().target;
    m_baseTarget = m_originalCameraTarget;

    m_collisionComponent.type = CollisionComponent::BOTH;
    m_collisionComponent.isStatic = false;
    m_collisionComponent.isActive = true;
    m_collisionComponent.sphere.radius = 3.5f;

    UpdateCollisionBounds();
}

Player::~Player() = default;

void Player::Update()
{
    ApplyInput();
    Jump();
    m_cameraController->Update();
    UpdatePositionHistory();
    UpdateCollisionBounds();
}

float Player::GetSpeed() { return m_walkSpeed; }

void Player::SetSpeed(const float speed) { this->m_walkSpeed = speed; }

void Player::Move(const Vector3 offset)
{
    m_cameraController->GetCamera().position =
        Vector3Add(m_cameraController->GetCamera().position, offset);
    m_cameraController->GetCamera().target =
        Vector3Add(m_cameraController->GetCamera().target, offset);
    UpdateCollisionBounds();
}

void Player::Jump()
{
    m_physData.m_dt = GetFrameTime();

    if (IsKeyPressed(KEY_SPACE) && m_physData.m_isGrounded)
    {
        m_physData.m_velocityY = m_jumpStrength * 0.8f; // Зменшено силу стрибка
        m_physData.m_isGrounded = false;
        m_isJumping = true;

        m_originalCameraTarget = m_cameraController->GetCamera().target;
        m_baseTarget = m_originalCameraTarget;
    }

    if (m_isJumping || !m_physData.m_isGrounded)
    {
        m_physData.m_velocityY -= m_physData.m_gravity * m_physData.m_dt;
        m_jumpOffsetY += m_physData.m_velocityY * m_physData.m_dt;

        if (m_jumpOffsetY <= 0.0f)
        {
            m_jumpOffsetY = 0.0f;
            m_physData.m_velocityY = 0.0f;
            m_physData.m_isGrounded = true;
            m_isJumping = false;
            m_cameraController->GetCamera().target = m_originalCameraTarget;
        }
        else
        {
            ApplyJumpToCamera(m_cameraController->GetCamera(), m_originalCameraTarget,
                              m_jumpOffsetY);
        }
    }
}

void Player::ApplyJumpToCamera(Camera &camera, const Vector3 &baseTarget, float jumpOffsetY)
{
    Vector3 desiredTarget = {baseTarget.x, baseTarget.y + jumpOffsetY, baseTarget.z};
    float smoothingSpeed = 2.0f; // Зменшено швидкість для плавнішого руху
    camera.target = Vector3Lerp(camera.target, desiredTarget, smoothingSpeed * GetFrameTime());
    camera.position = Vector3Lerp(camera.position, {camera.position.x, desiredTarget.y, camera.position.z}, smoothingSpeed * GetFrameTime());
}

void Player::UpdatePositionHistory()
{
    m_posData.m_playerVelocity =
        Vector3Subtract(m_posData.m_playerLastPosition, m_posData.m_playerCurrentPosition);
    m_posData.m_playerLastPosition = m_posData.m_playerCurrentPosition;
    m_posData.m_playerCurrentPosition = m_cameraController->GetCamera().position;
}

void Player::ApplyInput()
{
    m_physData.m_dt = GetFrameTime();
    Vector3 moveDir = {};

    if (IsKeyDown(KEY_W))
        moveDir.z -= 1.0f;
    if (IsKeyDown(KEY_S))
        moveDir.z += 1.0f;
    if (IsKeyDown(KEY_A))
        moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_D))
        moveDir.x += 1.0f;

    m_walkSpeed = IsKeyDown(KEY_LEFT_SHIFT) ? m_runSpeed : 3.1f;

    if (Vector3Length(moveDir) > 0)
    {
        moveDir = Vector3Normalize(moveDir);

        Vector3 forward = Vector3Subtract(GetCameraController()->GetCamera().position,
                                          GetCameraController()->GetCamera().target);
        forward.y = 0;
        forward = Vector3Normalize(forward);

        Vector3 right = Vector3CrossProduct((Vector3){0, 1, 0}, forward);
        right = Vector3Normalize(right);

        Vector3 finalMove = {right.x * moveDir.x + forward.x * moveDir.z, 0.0f,
                             right.z * moveDir.x + forward.z * moveDir.z};

        finalMove = Vector3Scale(finalMove, m_walkSpeed * m_physData.m_dt);
        Move(finalMove);
    }
}

std::shared_ptr<CameraController> Player::GetCameraController() const { return m_cameraController; }

Models Player::GetModelManager() { return m_modelPlayer; }

PositionData Player::GetPlayerData() const { return m_posData; }

Matrix Player::GetPlayerRotation()
{
    Vector3 playerPos = m_posData.m_playerCurrentPosition;
    Vector3 cameraTarget = m_cameraController->GetCamera().target;
    Vector3 toCamera = Vector3Subtract(cameraTarget, playerPos);
    float angleY = atan2f(toCamera.x, toCamera.z);
    return MatrixRotateY(angleY);
}

CollisionComponent &Player::GetCollisionComponent() { return m_collisionComponent; }

void Player::UpdateCollisionBounds()
{
    Vector3 cameraPos = m_cameraController->GetCamera().position;
    Vector3 center = {cameraPos.x, 0.5f + m_jumpOffsetY, cameraPos.z};

    m_collisionComponent.sphere.center = center;

    Vector3 halfSize = {0.5f, 0.5f, 0.5f};
    m_collisionComponent.box.min = Vector3Subtract(center, halfSize);
    m_collisionComponent.box.max = Vector3Add(center, halfSize);
}

void Player::SetCollisionRadius(float radius) { m_collisionComponent.sphere.radius = radius; }

void Player::SetCollisionSize(Vector3 size)
{
    Vector3 center = m_collisionComponent.sphere.center;
    Vector3 halfSize = Vector3Scale(size, 0.5f);
    m_collisionComponent.box.min = Vector3Subtract(center, halfSize);
    m_collisionComponent.box.max = Vector3Add(center, halfSize);
}

bool Player::CheckCollision(const CollisionComponent &other) const
{
    if (!m_collisionComponent.isActive || !other.isActive)
        return false;

    if (m_collisionComponent.type == CollisionComponent::SPHERE &&
        other.type == CollisionComponent::SPHERE)
    {
        return CollisionSystem::CheckSphereSphere(m_collisionComponent.sphere, other.sphere);
    }
    else if (m_collisionComponent.type == CollisionComponent::SPHERE &&
             other.type == CollisionComponent::AABB)
    {
        return CollisionSystem::CheckSphereAABB(m_collisionComponent.sphere, other.box);
    }
    else if (m_collisionComponent.type == CollisionComponent::AABB &&
             other.type == CollisionComponent::AABB)
    {
        return CollisionSystem::CheckAABBAABB(m_collisionComponent.box, other.box);
    }

    return false;
}

void Player::HandleCollisionResponse(const CollisionComponent &other)
{
    if (!CheckCollision(other))
        return;

    Vector3 response = {0, 0, 0};

    if (m_collisionComponent.type == CollisionComponent::SPHERE &&
        other.type == CollisionComponent::SPHERE)
        response =
            CollisionSystem::GetSphereSphereResponse(m_collisionComponent.sphere, other.sphere);
    else if (m_collisionComponent.type == CollisionComponent::SPHERE &&
             other.type == CollisionComponent::AABB)
        response = CollisionSystem::GetSphereAABBResponse(m_collisionComponent.sphere, other.box);

    if (Vector3Length(response) > 0.001f)
    {
        Move(response);
    }
}

void Player::CheckAndHandleCollisions(const std::vector<CollisionComponent *> &colliders)
{
    for (const auto &collider : colliders)
    {
        if (collider && collider->isActive)
        {
            HandleCollisionResponse(*collider);
        }
    }
}

bool Player::MoveWithCollisionDetection(Vector3 offset,
                                        const std::vector<CollisionComponent *> &colliders)
{
    Vector3 originalPosition = m_cameraController->GetCamera().position;
    Vector3 originalTarget = m_cameraController->GetCamera().target;

    Move(offset);

    for (const auto &collider : colliders)
    {
        if (collider && collider->isActive && CheckCollision(*collider))
        {
            m_cameraController->GetCamera().position = originalPosition;
            m_cameraController->GetCamera().target = originalTarget;
            UpdateCollisionBounds();
            return false;
        }
    }

    return true;
}
