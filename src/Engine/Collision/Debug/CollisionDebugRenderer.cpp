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

void CollisionDebugRenderer::RenderCollisionTriangles(const Collision &collision, Color color) const
{
    for (size_t i = 0; i < collision.GetTriangleCount(); ++i)
    {
        const auto& tri = collision.GetTriangle(i);
        DrawLine3D(tri.V0(), tri.V1(), color);
        DrawLine3D(tri.V1(), tri.V2(), color);
        DrawLine3D(tri.V2(), tri.V0(), color);
    }
}

void CollisionDebugRenderer::RenderAllCollisions(const std::vector<std::unique_ptr<Collision>> &collisions) const
{
    TraceLog(LOG_DEBUG, "CollisionDebugRenderer::RenderAllCollisions() - Rendering %zu collision objects", collisions.size());

    for (size_t i = 0; i < collisions.size(); i++)
    {
        Color color = (i == 0) ? m_groundColor : m_obstacleColor;
        TraceLog(LOG_DEBUG, "CollisionDebugRenderer::RenderAllCollisions() - Rendering collision %zu with color (%d,%d,%d,%d), triangles: %zu",
                 i, color.r, color.g, color.b, color.a, collisions[i]->GetTriangleCount());
        RenderCollisionBox(*collisions[i].get(), color);
        RenderCollisionTriangles(*collisions[i], RED);
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
void CollisionDebugRenderer::SetWireframeMode(bool wireframe) { m_wireframe = wireframe; }
