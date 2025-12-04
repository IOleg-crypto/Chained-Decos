#ifndef COLLISION_DEBUG_RENDERER_H
#define COLLISION_DEBUG_RENDERER_H

#include "../System/CollisionSystem.h"
#include <vector>
#include <raylib.h>

// Only Render collision
class CollisionDebugRenderer
{
public:
    CollisionDebugRenderer() = default;
    ~CollisionDebugRenderer() = default;

    void RenderCollisionBox(const Collision &collision, Color color = RED) const;
    void RenderAllCollisions(const std::vector<std::unique_ptr<Collision>> &collisions) const;
    void RenderPlayerCollision(const Collision &playerCollision) const;

    void RenderCollisionTriangles(const Collision &collision, Color color) const;

    void SetWireframeMode(bool wireframe);
    void SetDefaultColors(Color ground, Color obstacles, Color player);

private:
    bool m_wireframe = true;

    Color m_groundColor = GREEN;
    Color m_obstacleColor = RED;
    Color m_playerColor = BLUE;

    void DrawCollisionWireframe(const Vector3 &center, const Vector3 &size, Color color) const;
    void DrawCollisionSolid(const Vector3 &center, const Vector3 &size, Color color) const;
};

#endif
