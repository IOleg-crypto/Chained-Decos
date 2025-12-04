#ifndef PLAYER_H
#define PLAYER_H

#include "interfaces/ITransformable.h"
#include "interfaces/IUpdatable.h"
#include "physics/Core/Physics.h"

class Player : public Core::ITransformable, public Core::IUpdatable
{
public:
    Player();
    ~Player() override = default;

    // Interface implementation
    void Update(float deltaTime) override;

    Vector3 GetPosition() const override;
    void SetPosition(const Vector3 &pos) override;

    float GetRotationY() const override;
    void SetRotationY(float rotation) override;

    // Components
    Servers::Physics &GetPhysics();
    const Servers::Bounds &GetBounds() const;

    // Actions
    void Jump();
    void Move(const Vector3 &direction);

private:
    // State
    Servers::Physics m_physics;
    Servers::Bounds m_bounds;
    float m_rotationY = 0.0f;
    float m_speed = 5.0f;

    // Internal helpers
    void HandleInput();
    void UpdatePhysics(float dt);
};

#endif // PLAYER_H
