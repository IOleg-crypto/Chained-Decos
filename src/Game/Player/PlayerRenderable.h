#ifndef PLAYER_RENDERABLE_H
#define PLAYER_RENDERABLE_H

#include <Render/IRenderable.h>

// Forward declaration
class Player;

// PlayerRenderable - адаптер для Player, що реалізує IRenderable
// Використовується для уникнення множинного наслідування в Player
class PlayerRenderable : public IRenderable
{
public:
    explicit PlayerRenderable(Player* player);
    ~PlayerRenderable() = default;

    // IRenderable interface implementations
    void Update(CollisionManager& collisionManager) override;
    void Render() override;
    Vector3 GetPosition() const override;
    BoundingBox GetBoundingBox() const override;
    float GetRotationY() const override;
    void UpdateCollision() override;
    const Collision& GetCollision() const override;
    Camera GetCamera() const override;
    bool IsGrounded() const override;

private:
    Player* m_player;
};

#endif // PLAYER_RENDERABLE_H

