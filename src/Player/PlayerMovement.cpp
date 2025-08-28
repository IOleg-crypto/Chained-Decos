#include <Player/Player.h>
#include <Player/PlayerMovement.h>
#include <raylib.h>
#include <cmath>

PlayerMovement::PlayerMovement(Player *player)
    : m_player(player),
      m_position(Player::DEFAULT_SPAWN_POSITION),
      m_rotationY(0.0f),
      m_walkSpeed(11.0f),
      m_lastCollisionManager(nullptr)
{
    m_physics.SetGroundLevel(false);
    m_physics.SetVelocity({0, 0, 0});
}

void PlayerMovement::Move(const Vector3 &moveVector)
{
    m_position = Vector3Add(m_position, moveVector);
}

void PlayerMovement::SetPosition(const Vector3 &pos)
{
    m_position = pos;
    m_player->UpdatePlayerBox();
}

Vector3 PlayerMovement::GetPosition() const { return m_position; }

float PlayerMovement::GetRotationY() const { return m_rotationY; }

float PlayerMovement::GetSpeed() { return m_walkSpeed; }

void PlayerMovement::SetSpeed(float speed) { m_walkSpeed = speed; }

PhysicsComponent &PlayerMovement::GetPhysics() { return m_physics; }

const PhysicsComponent &PlayerMovement::GetPhysics() const { return m_physics; }

void PlayerMovement::SetCollisionManager(const CollisionManager *collisionManager)
{
    m_lastCollisionManager = collisionManager;
}

void PlayerMovement::ApplyJumpImpulse(float impulse)
{
    if (!m_physics.IsGrounded())
    {
        float vy = m_physics.GetVelocity().y;
        if (!(vy < 0.0f && vy > -0.3f)) return;
    }

    Vector3 vel = m_physics.GetVelocity();
    vel.y = impulse;
    m_physics.SetVelocity(vel);
    m_physics.SetGroundLevel(false);
    m_player->GetPhysics().SetJumpState(true);
}

void PlayerMovement::ApplyGravity(float deltaTime)
{
    Vector3 vel = m_physics.GetVelocity();
    if (!m_physics.IsGrounded())
    {
        vel.y -= m_physics.GetGravity() * deltaTime;
        constexpr float MAX_FALL_SPEED = -50.0f;
        if (vel.y < MAX_FALL_SPEED) vel.y = MAX_FALL_SPEED;
        m_physics.SetVelocity(vel);
    }
    else
    {
        if (vel.y < 0.0f) vel.y = 0.0f;
        m_physics.SetVelocity(vel);
    }
}

Vector3 PlayerMovement::StepMovement(const CollisionManager &collisionManager)
{
    Vector3 velocity = m_physics.GetVelocity();
    float deltaTime = GetFrameTime();
    Vector3 workingPos = GetPosition();

    constexpr int subSteps = 4;
    Vector3 subStepVelocity = Vector3Scale(velocity, deltaTime / subSteps);

    bool groundedThisStep = false;

    for (int i = 0; i < subSteps; i++)
    {
        Vector3 targetPos = Vector3Add(workingPos, subStepVelocity);
        SetPosition(targetPos);
        m_player->UpdatePlayerBox();

        Vector3 response;
        if (collisionManager.CheckCollision(m_player->GetCollision(), response))
        {
            HandleCollisionVelocity(response);

            if (response.y > 0.001f && velocity.y <= 0.0f)
                groundedThisStep = true;

            targetPos = Vector3Add(workingPos, Vector3Scale(response, 1.01f));
            SetPosition(targetPos);
            m_player->UpdatePlayerBox();
        }

        workingPos = GetPosition();
    }

    m_physics.SetGroundLevel(groundedThisStep);

    return workingPos;
}

void PlayerMovement::HandleCollisionVelocity(const Vector3 &responseMtv)
{
    Vector3 velocity = m_physics.GetVelocity();

    if (responseMtv.y > 0.001f && velocity.y <= 0.0f)
    {
        velocity.y = 0.0f;
        m_physics.SetGroundLevel(true);
    }
    else if (responseMtv.y < -0.001f && velocity.y > 0.0f)
    {
        velocity.y = 0.0f;
    }
    else
    {
        Vector3 normal = responseMtv;
        float len = Vector3Length(normal);
        if (len > 1e-6f)
            normal = Vector3Scale(normal, 1.0f / len);
        else
            normal = {0, 0, 0};

        float vn = Vector3DotProduct(velocity, normal);
        velocity = Vector3Subtract(velocity, Vector3Scale(normal, vn));
    }

    m_physics.SetVelocity(velocity);

    if (fabs(responseMtv.y) < 0.001f)
        m_physics.SetGroundLevel(false);
}

void PlayerMovement::SnapToGround(const CollisionManager &collisionManager)
{
    Vector3 velocity = m_physics.GetVelocity();
    if (velocity.y > 0.0f)
    {
        m_physics.SetGroundLevel(false);
        return;
    }

    constexpr float SNAP_DISTANCE = 0.35f;
    Vector3 checkPos = m_position;
    checkPos.y -= SNAP_DISTANCE;
    SetPosition(checkPos);
    m_player->UpdatePlayerBox();

    Vector3 response;
    bool collide = collisionManager.CheckCollision(m_player->GetCollision(), response);

    SetPosition(m_position);
    m_player->UpdatePlayerBox();

    if (collide && fabs(response.y) >= fabs(response.x) && fabs(response.y) >= fabs(response.z))
    {
        Vector3 pos = m_position;
        pos.y += response.y;
        SetPosition(pos);
        m_player->UpdatePlayerBox();
        velocity.y = 0.0f;
        m_physics.SetVelocity(velocity);
        m_physics.SetGroundLevel(true);
    }
    else
    {
        m_physics.SetGroundLevel(false);
    }
}

bool PlayerMovement::ExtractFromCollider()
{
    if (!m_lastCollisionManager) return false;

    Vector3 velocity = m_physics.GetVelocity();
    if (!m_physics.IsGrounded() || fabsf(velocity.y) > 0.1f) return false;

    Vector3 currentPos = GetPosition();
    m_player->UpdatePlayerBox();

    Vector3 response;
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), response))
        return false;

    Vector3 safePos = {currentPos.x, currentPos.y + 0.5f, currentPos.z};
    SetPosition(safePos);
    m_player->UpdatePlayerBox();

    Vector3 testResponse;
    if (!m_lastCollisionManager->CheckCollision(m_player->GetCollision(), testResponse))
        return true;

    SetPosition(Player::DEFAULT_SPAWN_POSITION);
    m_physics.SetVelocity({0, 0, 0});
    m_physics.SetGroundLevel(false);
    return true;
}

void PlayerMovement::SetRotationY(float rotation) { m_rotationY = rotation; }
