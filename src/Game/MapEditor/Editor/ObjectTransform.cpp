//
// Created by AI Assistant
//

#include "ObjectTransform.h"
#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <cfloat>

void ObjectTransform::DrawGizmo(Vector3 position, Vector3 scale, Vector3 rotation, GizmoMode mode)
{
    switch (mode)
    {
    case GizmoMode::TRANSLATE:
        DrawTranslateGizmo(position);
        break;
    case GizmoMode::ROTATE:
        DrawRotateGizmo(position);
        break;
    case GizmoMode::SCALE:
        DrawScaleGizmo(position, scale);
        break;
    }
}

bool ObjectTransform::HandleGizmoInteraction(Vector3& position, Vector3& scale, Vector3& rotation, GizmoMode mode)
{
    switch (mode)
    {
    case GizmoMode::TRANSLATE:
        return HandleTranslateGizmo(position);
    case GizmoMode::ROTATE:
        return HandleRotateGizmo(rotation);
    case GizmoMode::SCALE:
        return HandleScaleGizmo(scale);
    }
    return false;
}

Matrix ObjectTransform::GetTransformMatrix(Vector3 position, Vector3 scale, Vector3 rotation)
{
    Matrix translation = MatrixTranslate(position.x, position.y, position.z);
    Matrix scaleMatrix = MatrixScale(scale.x, scale.y, scale.z);
    
    Matrix rotationX = MatrixRotateX(rotation.x);
    Matrix rotationY = MatrixRotateY(rotation.y);
    Matrix rotationZ = MatrixRotateZ(rotation.z);
    
    Matrix rotationMatrix = MatrixMultiply(MatrixMultiply(rotationX, rotationY), rotationZ);
    
    return MatrixMultiply(MatrixMultiply(scaleMatrix, rotationMatrix), translation);
}

Vector3 ObjectTransform::WorldToScreen(Vector3 worldPos, Camera camera, int screenWidth, int screenHeight)
{
    Vector2 screenPos = GetWorldToScreen(worldPos, camera);
    return {screenPos.x, screenPos.y, 0.0f};
}

Vector3 ObjectTransform::ScreenToWorld(Vector2 screenPos, Camera camera, int screenWidth, int screenHeight)
{
    Ray ray = GetScreenToWorldRay(screenPos, camera);
    
    // Find intersection with ground plane (y = 0)
    if (ray.direction.y != 0)
    {
        float t = -ray.position.y / ray.direction.y;
        return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
    }
    
    return ray.position;
}

int ObjectTransform::PickObject(const std::vector<Vector3>& positions, const std::vector<Vector3>& scales, 
                               const std::vector<int>& types, Camera camera, Vector2 mousePos)
{
    Ray ray = GetScreenToWorldRay(mousePos, camera);
    int pickedIndex = -1;
    float minDistance = FLT_MAX;

    for (size_t i = 0; i < positions.size(); ++i)
    {
        if (i >= types.size() || i >= scales.size())
            continue;

        BoundingBox box = {
            Vector3{positions[i].x - scales[i].x * 0.5f, positions[i].y - scales[i].y * 0.5f, positions[i].z - scales[i].z * 0.5f},
            Vector3{positions[i].x + scales[i].x * 0.5f, positions[i].y + scales[i].y * 0.5f, positions[i].z + scales[i].z * 0.5f}
        };

        RayCollision collision = GetRayCollisionBox(ray, box);
        if (collision.hit && collision.distance < minDistance)
        {
            minDistance = collision.distance;
            pickedIndex = static_cast<int>(i);
        }
    }

    return pickedIndex;
}

void ObjectTransform::DrawTranslateGizmo(Vector3 position)
{
    float gizmoSize = 2.0f;
    float axisLength = 1.5f;
    
    // X-axis (Red)
    DrawLine3D(position, Vector3Add(position, {axisLength, 0, 0}), RED);
    DrawCylinder(Vector3Add(position, {axisLength, 0, 0}), 0.05f, 0.05f, 0.2f, 8, RED);
    
    // Y-axis (Green)
    DrawLine3D(position, Vector3Add(position, {0, axisLength, 0}), GREEN);
    DrawCylinder(Vector3Add(position, {0, axisLength, 0}), 0.05f, 0.05f, 0.2f, 8, GREEN);
    
    // Z-axis (Blue)
    DrawLine3D(position, Vector3Add(position, {0, 0, axisLength}), BLUE);
    DrawCylinder(Vector3Add(position, {0, 0, axisLength}), 0.05f, 0.05f, 0.2f, 8, BLUE);
    
    // Center sphere
    DrawSphere(position, 0.1f, WHITE);
}

void ObjectTransform::DrawRotateGizmo(Vector3 position)
{
    float radius = 1.0f;
    int segments = 32;
    
    // X-axis rotation ring (Red)
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = (float)i * 2.0f * PI / segments;
        float angle2 = (float)(i + 1) * 2.0f * PI / segments;
        
        Vector3 p1 = {position.x, position.y + radius * cosf(angle1), position.z + radius * sinf(angle1)};
        Vector3 p2 = {position.x, position.y + radius * cosf(angle2), position.z + radius * sinf(angle2)};
        
        DrawLine3D(p1, p2, RED);
    }
    
    // Y-axis rotation ring (Green)
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = (float)i * 2.0f * PI / segments;
        float angle2 = (float)(i + 1) * 2.0f * PI / segments;
        
        Vector3 p1 = {position.x + radius * cosf(angle1), position.y, position.z + radius * sinf(angle1)};
        Vector3 p2 = {position.x + radius * cosf(angle2), position.y, position.z + radius * sinf(angle2)};
        
        DrawLine3D(p1, p2, GREEN);
    }
    
    // Z-axis rotation ring (Blue)
    for (int i = 0; i < segments; ++i)
    {
        float angle1 = (float)i * 2.0f * PI / segments;
        float angle2 = (float)(i + 1) * 2.0f * PI / segments;
        
        Vector3 p1 = {position.x + radius * cosf(angle1), position.y + radius * sinf(angle1), position.z};
        Vector3 p2 = {position.x + radius * cosf(angle2), position.y + radius * sinf(angle2), position.z};
        
        DrawLine3D(p1, p2, BLUE);
    }
    
    // Center sphere
    DrawSphere(position, 0.1f, WHITE);
}

void ObjectTransform::DrawScaleGizmo(Vector3 position, Vector3 scale)
{
    float gizmoSize = 2.0f;
    float axisLength = 1.5f;
    
    // X-axis (Red)
    DrawLine3D(position, Vector3Add(position, {axisLength, 0, 0}), RED);
    DrawCube(Vector3Add(position, {axisLength, 0, 0}), 0.2f, 0.2f, 0.2f, RED);
    
    // Y-axis (Green)
    DrawLine3D(position, Vector3Add(position, {0, axisLength, 0}), GREEN);
    DrawCube(Vector3Add(position, {0, axisLength, 0}), 0.2f, 0.2f, 0.2f, GREEN);
    
    // Z-axis (Blue)
    DrawLine3D(position, Vector3Add(position, {0, 0, axisLength}), BLUE);
    DrawCube(Vector3Add(position, {0, 0, axisLength}), 0.2f, 0.2f, 0.2f, BLUE);
    
    // Center cube
    DrawCube(position, 0.2f, 0.2f, 0.2f, WHITE);
}

bool ObjectTransform::HandleTranslateGizmo(Vector3& position)
{
    // This would need to be implemented with proper mouse interaction
    // For now, return false as this is a placeholder
    return false;
}

bool ObjectTransform::HandleRotateGizmo(Vector3& rotation)
{
    // This would need to be implemented with proper mouse interaction
    // For now, return false as this is a placeholder
    return false;
}

bool ObjectTransform::HandleScaleGizmo(Vector3& scale)
{
    // This would need to be implemented with proper mouse interaction
    // For now, return false as this is a placeholder
    return false;
}