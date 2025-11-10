#ifndef PLAYER_RENDERABLE_H
#define PLAYER_RENDERABLE_H

#include <Render/Interfaces/IGameRenderable.h>

// Forward declaration
class Player;

class PlayerRenderable : public IGameRenderable
{
public:
    explicit PlayerRenderable(Player* player);
    ~PlayerRenderable() = default;

    // IGameRenderable interface implementations
    void Update(CollisionManager& collisionManager) override;
    Vector3 GetPosition() const override;
    BoundingBox GetBoundingBox() const override;
    float GetRotationY() const override;
    void UpdateCollision() override;
    const Collision& GetCollision() const override;
    Camera GetCamera() const override;
    bool IsGrounded() const override;
    float GetVelocityY() const override;

private:
    Player* m_player;
};

#endif // PLAYER_RENDERABLE_H

