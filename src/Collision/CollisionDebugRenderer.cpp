#include "CollisionDebugRenderer.h"
#include <raylib.h>

void CollisionDebugRenderer::RenderCollisionBox(const Collision &collision, Color color) const
{
    Vector3 center = collision.GetCenter();
    Vector3 size = collision.GetSize();

    if (m_wireframe)
    {
        DrawCollisionWireframe(center, size, color);
    }
    else
    {
        DrawCollisionSolid(center, size, color);
    }
}

void CollisionDebugRenderer::RenderAllCollisions(const std::vector<Collision> &collisions) const
{
    for (size_t i = 0; i < collisions.size(); i++)
    {
        Color color = (i == 0) ? m_groundColor : m_obstacleColor; //
        RenderCollisionBox(collisions[i], color);
    }
}

void CollisionDebugRenderer::RenderPlayerCollision(const Collision &playerCollision) const
{
    RenderCollisionBox(playerCollision, m_playerColor);
}

void CollisionDebugRenderer::SetDefaultColors(Color ground, Color obstacles, Color player)
{
    m_groundColor = ground;
    m_obstacleColor = obstacles;
    m_playerColor = player;
}

void CollisionDebugRenderer::DrawCollisionWireframe(const Vector3 &center, const Vector3 &size,
                                                    Color color) const
{
    DrawCubeWires(center, size.x, size.y, size.z, color);
}

void CollisionDebugRenderer::DrawCollisionSolid(const Vector3 &center, const Vector3 &size,
                                                Color color) const
{
    Color transparentColor = {color.r, color.g, color.b, 80};
    DrawCube(center, size.x, size.y, size.z, transparentColor);
    DrawCubeWires(center, size.x, size.y, size.z, color);
}