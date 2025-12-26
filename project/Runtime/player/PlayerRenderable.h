#ifndef PLAYER_RENDERABLE_H
#define PLAYER_RENDERABLE_H

#include "core/interfaces/IPlayer.h"
#include <components/rendering/interfaces/IGameRenderable.h>

class PlayerRenderable : public IGameRenderable
{
public:
    PlayerRenderable() = default;
    ~PlayerRenderable() = default;

public:
    // IGameRenderable interface implementations - all accept player reference
    void Update(IPlayer &player, CollisionManager &collisionManager);
    Vector3 GetPosition(const IPlayer &player) const;
    BoundingBox GetBoundingBox(const IPlayer &player) const;
    float GetRotationY(const IPlayer &player) const;
    void UpdateCollision(IPlayer &player);
    const Collision &GetCollision(const IPlayer &player) const;
    Camera GetCamera(IPlayer &player);
    bool IsGrounded(const IPlayer &player) const;
    float GetVelocityY(const IPlayer &player) const;

public:
    // Legacy IGameRenderable overrides (delegates to player-accepting versions)
    void Update(CollisionManager &collisionManager) override;
    Vector3 GetPosition() const override;
    BoundingBox GetBoundingBox() const override;
    float GetRotationY() const override;
    void UpdateCollision() override;
    const Collision &GetCollision() const override;
    Camera GetCamera() const override;
    bool IsGrounded() const override;
    float GetVelocityY() const override;

    void SetPlayer(IPlayer *player);

private:
    IPlayer *m_player = nullptr;
};

#endif // PLAYER_RENDERABLE_H






